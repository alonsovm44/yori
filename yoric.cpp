/* YORI COMPILER (yori.exe) - v4.2 (Universal Polyglot)
   Usage: yori file.yori [-o output] [FLAGS]
   Supported: C++, C, Rust, Go, Zig, Swift, Java, C#, Python, JS, TS, Ruby, PHP, Lua, Perl, R, Julia, Haskell, Bash, PowerShell.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <cstdio>  
#include <cstdlib> 
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
#include <iomanip>

#include "json.hpp" 

using json = nlohmann::json;
using namespace std;

// --- CONFIGURATION ---
string PROVIDER = "local"; 
string API_KEY = "";
string MODEL_ID = ""; 
string API_URL = "";
const int MAX_RETRIES = 5;

// --- LANGUAGE SYSTEM ---
struct LangProfile {
    string id;         
    string name;       
    string extension;  
    string versionCmd; // To check installation
    string buildCmd;   // To check syntax/build
    bool producesBinary; 
};

// MASSIVE LANGUAGE DB
map<string, LangProfile> LANG_DB = {
    // SYSTEMS
    {"cpp",  {"cpp", "C++", ".cpp", "g++ --version", "g++ -std=c++17", true}},
    {"c",    {"c",   "C",   ".c",   "gcc --version", "gcc", true}},
    {"rust", {"rust","Rust",".rs",  "rustc --version", "rustc --crate-type bin", true}},
    {"go",   {"go",  "Go",  ".go",  "go version", "go build", true}},
    {"zig",  {"zig", "Zig", ".zig", "zig version", "zig build-exe", true}},
    {"swift",{"swift","Swift",".swift","swiftc --version", "swiftc", true}},
    
    // ENTERPRISE / JVM / .NET
    {"java", {"java","Java",".java","javac -version", "javac", false}}, // Class files are binaries but treated as source for yori
    {"cs",   {"cs",  "C#",  ".cs",  "dotnet --version", "dotnet build", true}}, // Assumes dotnet core project context usually, but simple csc works too if installed. We stick to dotnet.
    {"scala",{"scala","Scala",".scala","scalac -version", "scalac", false}},

    // SCRIPTING / WEB
    {"py",   {"py",  "Python", ".py", "python --version", "python -m py_compile", false}},
    {"js",   {"js",  "JavaScript", ".js", "node --version", "node -c", false}},
    {"ts",   {"ts",  "TypeScript", ".ts", "tsc --version", "tsc --noEmit", false}},
    {"rb",   {"rb",  "Ruby", ".rb",   "ruby --version", "ruby -c", false}},
    {"php",  {"php", "PHP", ".php",   "php --version", "php -l", false}},
    {"lua",  {"lua", "Lua", ".lua",   "luac -v", "luac -p", false}},
    {"pl",   {"pl",  "Perl", ".pl",   "perl --version", "perl -c", false}},

    // DATA / FUNCTIONAL
    {"r",    {"r",   "R", ".R",       "R --version", "R CMD BATCH --no-save --no-restore", false}}, 
    {"hs",   {"hs",  "Haskell", ".hs","ghc --version", "ghc -fno-code", false}}, // -fno-code just checks logic
    {"jl",   {"jl",  "Julia", ".jl",  "julia -v", "julia", false}},
    // SHELL
    {"sh",   {"sh",  "Bash", ".sh",   "bash --version", "bash -n", false}},
    {"ps1",  {"ps1", "PowerShell", ".ps1", "pwsh -v", "pwsh -Command Get-Date", false}} // pwsh checks install
};

LangProfile CURRENT_LANG; 

// --- UTILS ---
#ifdef _WIN32
extern "C" { FILE* _popen(const char* command, const char* mode); int _pclose(FILE* stream); }
#else
    #define _popen popen
    #define _pclose pclose
#endif

string execCmd(string cmd) {
    array<char, 128> buffer;
    string result;
    string full_cmd = cmd + " 2>&1"; 
    FILE* pipe = _popen(full_cmd.c_str(), "r");
    if (!pipe) return "EXEC_FAIL";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) result += buffer.data();
    _pclose(pipe);
    return result;
}

string stripExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? fname : fname.substr(0, lastindex); 
}

string getExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? "" : fname.substr(lastindex); 
}

// --- CONFIG LOADER ---
bool loadConfig(string mode) {
    ifstream f("config.json");
    if (!f.is_open()) { cerr << "FATAL: config.json missing." << endl; return false; }
    try {
        json j = json::parse(f);
        if (!j.contains(mode)) return false;
        json profile = j[mode];
        PROVIDER = mode;
        if (mode == "cloud") API_KEY = profile["api_key"];
        MODEL_ID = profile["model_id"];
        if (mode == "local") API_URL = profile.contains("api_url") ? profile["api_url"].get<string>() : "http://localhost:11434/api/generate";
        return true;
    } catch (...) { return false; }
}

// --- AI INTERFACE ---
string callAI(string prompt) {
    string response;
    while (true) {
        json body; string url;
        if (PROVIDER == "local") { body["model"]=MODEL_ID; body["prompt"]=prompt; body["stream"]=false; url=API_URL; }
        else { body["contents"][0]["parts"][0]["text"]=prompt; url="https://generativelanguage.googleapis.com/v1beta/models/"+MODEL_ID+":generateContent?key="+API_KEY; }
        ofstream file("request_temp.json"); file << body.dump(); file.close();
        string cmd = "curl -s -X POST -H \"Content-Type: application/json\" -d @request_temp.json \"" + url + "\"";
        response = execCmd(cmd); remove("request_temp.json");
        if (PROVIDER == "cloud" && response.find("429") != string::npos) { this_thread::sleep_for(chrono::seconds(30)); continue; }
        break;
    }
    return response;
}

string extractCode(string jsonResponse) {
    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        if (j.contains("error")) return "ERROR: API Error";
        if (PROVIDER == "local") { if (j.contains("response")) raw = j["response"]; } 
        else { if (j.contains("candidates")) raw = j["candidates"][0]["content"]["parts"][0]["text"]; }
        size_t start = raw.find("```"); if (start == string::npos) return raw;
        size_t end_line = raw.find('\n', start); size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos) return raw.substr(end_line + 1, end_block - end_line - 1);
        return raw;
    } catch (...) { return "JSON_PARSE_ERROR"; }
}

// --- INTERACTIVE MENU ---
void selectLanguage() {
    cout << "\n[?] Ambiguous output. Please select target language:\n" << endl;
    
    vector<string> keys;
    for(auto const& [key, val] : LANG_DB) keys.push_back(key);
    
    // Simple 3-column layout
    int i = 1;
    for (const auto& key : keys) {
        cout << std::left << std::setw(4) << i << std::setw(15) << (LANG_DB[key].name + " (" + LANG_DB[key].id + ")");
        if (i % 3 == 0) cout << endl;
        i++;
    }
    cout << "\n\n> Selection [1-" << keys.size() << "]: ";
    
    int choice;
    if (!(cin >> choice)) choice = 1;
    if (choice < 1 || choice > keys.size()) choice = 1;
    
    CURRENT_LANG = LANG_DB[keys[choice-1]];
    
    // Clear buffer
    string dummy; getline(cin, dummy); 
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: yori <file.yori> [-o output] [FLAGS]" << endl;
        return 0;
    }

    string inputFile = argv[1];
    string outputName = "";
    string mode = "local"; 
    bool explicitLang = false;
    bool updateMode = false;

    // 1. ARGUMENT PARSING
    for(int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-cloud") mode = "cloud";
        if (arg == "-local") mode = "local";
        if (arg == "-u") updateMode = true;
        if (arg == "-o" && i+1 < argc) outputName = argv[i+1];

        // Flag detection (e.g. -rust, -zig, -ts)
        if (arg[0] == '-') {
            string cleanArg = arg.substr(1); 
            if (LANG_DB.count(cleanArg)) {
                CURRENT_LANG = LANG_DB[cleanArg];
                explicitLang = true;
            }
        }
    }

    if (!loadConfig(mode)) return 1;

    // 2. LANGUAGE NEGOTIATION
    if (!explicitLang) {
        if (outputName.empty()) {
             cout << "[INFO] No lang specified. Defaulting to C++." << endl;
             CURRENT_LANG = LANG_DB["cpp"];
        } else {
            string ext = getExt(outputName);
            bool found = false;
            // Scan DB for extension match
            for (auto const& [key, val] : LANG_DB) {
                if (val.extension == ext) {
                    CURRENT_LANG = val;
                    found = true;
                    break;
                }
            }
            if (!found) selectLanguage();
            else cout << "[INFO] Detected: " << CURRENT_LANG.name << endl;
        }
    }

    if (outputName.empty()) outputName = stripExt(inputFile) + CURRENT_LANG.extension;

    // 3. HEALTH CHECK
    cout << "[CHECK] Looking for " << CURRENT_LANG.name << " toolchain..." << endl;
    string verCheck = execCmd(CURRENT_LANG.versionCmd);
    bool toolchainActive = true;

    if (verCheck.find("not recognized") != string::npos || 
        verCheck.find("not found") != string::npos || 
        verCheck == "EXEC_FAIL") {
        
        cerr << "\n[MISSING TOOLS] '" << CURRENT_LANG.versionCmd << "' failed." << endl;
        cout << "Continue in BLIND MODE? [y/N]: ";
        char ans; cin >> ans;
        if (ans != 'y' && ans != 'Y') return 1;
        toolchainActive = false;
    } else {
        cout << "   [OK] Found." << endl;
    }

    // 4. GENERATION LOOP
    ifstream f(inputFile);
    if (!f.is_open()) { cerr << "Error: Input missing." << endl; return 1; }
    string rawCode((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    string finalLogic = rawCode; 

    // Handle existing code for -u
    string existingCode = "";
    if (updateMode) {
        ifstream old(outputName);
        if (old.is_open()) existingCode = string((istreambuf_iterator<char>(old)), istreambuf_iterator<char>());
    }

    string tempFile = "temp_src" + CURRENT_LANG.extension;
    string currentError = "";
    int passes = toolchainActive ? MAX_RETRIES : 1;

    for(int gen=1; gen<=passes; gen++) {
        cout << "   [Pass " << gen << "] Writing " << CURRENT_LANG.name << "..." << endl;
        
        string prompt;
        if (currentError.empty()) {
            if (updateMode && !existingCode.empty()) {
                prompt = "ROLE: Expert " + CURRENT_LANG.name + " Dev.\nTASK: Update existing code.\n[OLD CODE]:\n" + existingCode + "\n[CHANGES]:\n" + finalLogic + "\nOUTPUT: Full " + CURRENT_LANG.name + " code.";
            } else {
                prompt = "ROLE: Expert " + CURRENT_LANG.name + " Dev.\nTASK: Write single-file program.\nINSTRUCTIONS:\n" + finalLogic + "\nOUTPUT: Valid " + CURRENT_LANG.name + " code only.";
            }
        } else {
            prompt = "ROLE: Expert Debugger.\nTASK: Fix " + CURRENT_LANG.name + " syntax.\nERROR:\n" + currentError + "\nCODE:\n" + finalLogic + "\nOUTPUT: Corrected code.";
        }

        string response = callAI(prompt);
        string code = extractCode(response);

        if (code.find("ERROR:") == 0) { cerr << "AI Error: " << code << endl; return 1; }
        
        ofstream out(tempFile); out << code; out.close();

        if (!toolchainActive) {
            remove(outputName.c_str()); rename(tempFile.c_str(), outputName.c_str());
            cout << "\nSaved (Blind): " << outputName << endl;
            return 0;
        }

        cout << "   Verifying..." << endl;
        string valCmd = CURRENT_LANG.buildCmd + " " + tempFile; 
        if (CURRENT_LANG.producesBinary && (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c" || CURRENT_LANG.id == "go")) {
             valCmd += " -o temp_check.exe";
        }
        // KEY CORRECTION:
        // Si es C++ o C, g++ necesita saber explícitamente dónde poner el exe temporal
        // para que no choque con nuestros archivos.
        string tempBin = "temp_check.exe";
        if (CURRENT_LANG.producesBinary && (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c" || CURRENT_LANG.id == "go" || CURRENT_LANG.id == "rust")) {
             // Rust usa -o de forma distinta, pero para g++ es crucial
             if (CURRENT_LANG.id == "rust") valCmd += " -o " + tempBin;
             else valCmd += " -o " + tempBin;
        }

        string output = execCmd(valCmd);
        
        // Success heuristic
        bool success = (output.find("error") == string::npos && output.find("Error") == string::npos && output.find("Exception") == string::npos);
        // Special case for languages that output errors to stdout/stderr differently
        if (CURRENT_LANG.id == "php" && output.find("No syntax errors") == string::npos) success = false;

        if (success) {
            cout << "\nBUILD SUCCESSFUL: " << outputName << endl;
            
            // SAVING LOGIC CORRECTED
            
            // CASO 1: COMPILED LANGUAGES (C++, Rust, Go, Zig)
            // El usuario quiere el .exe final, no el código fuente (usualmente).
            if (CURRENT_LANG.producesBinary) {
                // Borramos cualquier basura anterior
                remove(outputName.c_str());
                
                // Renombramos el binario temporal generado por el compilador al nombre final
                if (rename(tempBin.c_str(), outputName.c_str()) != 0) {
                    cerr << "Error moving binary to " << outputName << endl;
                    // Fallback: copiar si rename falla (común en Windows entre discos)
                    string copyCmd = "copy /Y " + tempBin + " " + outputName + " > NUL";
                    system(copyCmd.c_str());
                }
                
                // Opcional: ¿Guardar también el código fuente?
                // Muchos usuarios de Yori querrán ver el código generado.
                // Guardémoslo como example.cpp
                string sourceName = stripExt(outputName) + CURRENT_LANG.extension;
                remove(sourceName.c_str());
                rename(tempFile.c_str(), sourceName.c_str());
                
                cout << "   [Binary]: " << outputName << endl;
                cout << "   [Source]: " << sourceName << endl;
            } 
            // CASO 2: Lenguajes Interpretados/Scripts (Python, JS, etc)
            else {
                remove(outputName.c_str());
                rename(tempFile.c_str(), outputName.c_str());
                cout << "   [Script]: " << outputName << endl;
            }
            
            // Limpieza final
            remove(tempBin.c_str());
            return 0;
        }
        currentError = output;
    }

    cerr << "\nFAILED to generate valid code." << endl;
    return 1;
}