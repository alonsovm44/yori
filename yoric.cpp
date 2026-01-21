/* YORI COMPILER (yori.exe) - v4.5 (Multi-File + Error Memory + Update Mode)
   Usage: yori source1.ext source2.ext [-o output] [-u] [FLAGS]
   Features: 
     - Multi-file ingestion
     - Smart Import Resolution
     - AI Error Memory
     - Configurable Toolchains
     - Update Mode (-u) for iterative dev
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
#include <filesystem>
#include <ctime>
#include <functional>
#include <set>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "json.hpp" 

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

// --- CONFIGURATION ---
string PROVIDER = "local"; 
string API_KEY = "";
string MODEL_ID = ""; 
string API_URL = "";
const int MAX_RETRIES = 5;
bool VERBOSE_MODE = false;

// --- LOGGER SYSTEM ---
ofstream logFile;

void initLogger() {
    logFile.open("yori.log", ios::app); 
    if (logFile.is_open()) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        logFile << "\n--- SESSION START (v4.5): " << put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ---\n";
    }
}

void log(string level, string message) {
    if (logFile.is_open()) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        logFile << "[" << put_time(&tm, "%H:%M:%S") << "] [" << level << "] " << message << endl;
    }
    if (VERBOSE_MODE) cout << "   [" << level << "] " << message << endl;
}

// --- LANGUAGE SYSTEM ---
struct LangProfile {
    string id; string name; string extension;  
    string versionCmd; string buildCmd; bool producesBinary; 
};

map<string, LangProfile> LANG_DB = {
    {"cpp",  {"cpp", "C++", ".cpp", "g++ --version", "g++ -std=c++17", true}},
    {"c",    {"c",   "C",   ".c",   "gcc --version", "gcc", true}},
    {"rust", {"rust","Rust",".rs",  "rustc --version", "rustc", true}}, // rustc output handling varies
    {"go",   {"go",  "Go",  ".go",  "go version", "go build", true}},
    {"py",   {"py",  "Python", ".py", "python --version", "python -m py_compile", false}},
    {"js",   {"js",  "JavaScript", ".js", "node --version", "node -c", false}},
    {"ts",   {"ts",  "TypeScript", ".ts", "tsc --version", "tsc --noEmit", false}},
    {"cs",   {"cs",  "C#",  ".cs",  "dotnet --version", "dotnet build", true}},
    {"java", {"java","Java",".java","javac -version", "javac", false}}
};

LangProfile CURRENT_LANG; 

// --- UTILS ---
#ifdef _WIN32
#else
    #define _popen popen
    #define _pclose pclose
#endif

// Safe execution with path handling
struct CmdResult { string output; int exitCode; };

CmdResult execCmd(string cmd) {
    array<char, 128> buffer; string result; 
    string full_cmd = cmd + " 2>&1"; // Capture stderr
    
    FILE* pipe = _popen(full_cmd.c_str(), "r");
    if (!pipe) return {"EXEC_FAIL", -1};
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) result += buffer.data();
    int code = _pclose(pipe);
    return {result, code};
}

string stripExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? fname : fname.substr(0, lastindex); 
}

string getExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? "" : fname.substr(lastindex); 
}

// --- CONFIG & TOOLCHAIN OVERRIDES ---
bool loadConfig(string mode) {
    string configPath = "config.json";
    // (Logic to find config relative to exe omitted for brevity, assumes CWD or Env)
    
    ifstream f(configPath);
    if (!f.is_open()) {
        // Fallback defaults if no config
        if(mode == "local") API_URL = "http://localhost:11434/api/generate";
        return true; 
    }

    try {
        json j = json::parse(f);
        if (j.contains(mode)) {
            json profile = j[mode];
            PROVIDER = mode;
            if (mode == "cloud") API_KEY = profile.value("api_key", "");
            MODEL_ID = profile.value("model_id", "gemini-pro");
            if (mode == "local") API_URL = profile.value("api_url", "http://localhost:11434/api/generate");
        }

        // Feature: Custom Toolchains via Config
        if (j.contains("toolchains")) {
            for (auto& [key, val] : j["toolchains"].items()) {
                if (LANG_DB.count(key)) {
                    if (val.contains("build_cmd")) LANG_DB[key].buildCmd = val["build_cmd"];
                    if (val.contains("version_cmd")) LANG_DB[key].versionCmd = val["version_cmd"];
                    log("CONFIG", "Overrode toolchain for: " + key);
                }
            }
        }
        return true;
    } catch (...) { return false; }
}

// --- PREPROCESSOR (Improved v4.5) ---
// Returns: Processed content with explicit markers for the AI
string resolveImports(string code, fs::path basePath, vector<string>& stack) {
    stringstream ss(code);
    string line;
    string processed;
    
    while (getline(ss, line)) {
        string cleanLine = line;
        // Trim leading spaces
        size_t first = cleanLine.find_first_not_of(" \t\r\n");
        if (first == string::npos) { processed += line + "\n"; continue; }
        cleanLine.erase(0, first);
        
        if (cleanLine.rfind("IMPORT:", 0) == 0) {
            string fname = cleanLine.substr(7);
            // Quote removal logic
            size_t q1 = fname.find_first_of("\"'");
            size_t q2 = fname.find_last_of("\"'");
            if (q1 != string::npos && q2 != string::npos && q2 > q1) fname = fname.substr(q1 + 1, q2 - q1 - 1);
            else {
                fname.erase(0, fname.find_first_not_of(" \t\r\n\"'"));
                size_t last = fname.find_last_not_of(" \t\r\n\"'");
                if (last != string::npos) fname.erase(last + 1);
            }
            
            fs::path path = basePath / fname;
            
            try {
                if (fs::exists(path)) {
                    string absPath = fs::canonical(path).string();
                    
                    // Cycle detection
                    bool cycle = false;
                    for(const auto& s : stack) if(s == absPath) cycle = true;
                    
                    if (cycle) {
                        processed += "// [ERROR] CYCLIC IMPORT DETECTED: " + fname + "\n";
                        log("ERROR", "Circular import: " + fname);
                    } else {
                        ifstream imp(path);
                        if (imp.is_open()) {
                            string content((istreambuf_iterator<char>(imp)), istreambuf_iterator<char>());
                            stack.push_back(absPath);
                            string nested = resolveImports(content, path.parent_path(), stack);
                            stack.pop_back();
                            
                            // IMPROVEMENT: Explicit AI Context Markers
                            string ext = path.extension().string();
                            processed += "\n// >>>>>> START MODULE: " + fname + " (" + ext + ") >>>>>>\n";
                            processed += nested;
                            processed += "\n// <<<<<< END MODULE: " + fname + " <<<<<<\n";
                            
                            log("INFO", "Imported module: " + fname);
                        }
                    }
                } else {
                    processed += "// [WARN] IMPORT NOT FOUND: " + fname + "\n";
                }
            } catch (...) { processed += "// [ERROR] PATH EXCEPTION\n"; }
        } else {
            processed += line + "\n";
        }
    }
    return processed;
}

// --- AI CORE ---
string callAI(string prompt) {
    string response;
    // Retry mechanism for API failures (Network/RateLimit) - distinct from Compiler Error Retries
    for(int i=0; i<3; i++) {
        json body; string url;
        if (PROVIDER == "local") { 
            body["model"]=MODEL_ID; body["prompt"]=prompt; body["stream"]=false; url=API_URL; 
        } else { 
            body["contents"][0]["parts"][0]["text"]=prompt; 
            url="https://generativelanguage.googleapis.com/v1beta/models/"+MODEL_ID+":generateContent?key="+API_KEY; 
        }
        
        ofstream file("request_temp.json"); 
        file << body.dump(-1, ' ', false, json::error_handler_t::replace); 
        file.close();
        
        // Silent curl unless verbose
        string cmd = "curl -s -X POST -H \"Content-Type: application/json\" -d @request_temp.json \"" + url + "\"";
        CmdResult res = execCmd(cmd);
        response = res.output;
        remove("request_temp.json");
        
        if (PROVIDER == "cloud" && response.find("429") != string::npos) { 
             log("WARN", "API 429 Rate Limit. Backoff...");
             this_thread::sleep_for(chrono::seconds(5 * (i+1)));
             continue; 
        }
        break;
    }
    return response;
}

string extractCode(string jsonResponse) {
    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        
        // Handle Google / Ollama differences
        if (j.contains("error")) return "ERROR: " + j["error"].dump();
        if (PROVIDER == "local") { if (j.contains("response")) raw = j["response"]; } 
        else { if (j.contains("candidates")) raw = j["candidates"][0]["content"]["parts"][0]["text"]; }
        
        // Markdown extraction
        size_t start = raw.find("```"); 
        if (start == string::npos) return raw; // Fallback: return raw text if no code block
        
        size_t end_line = raw.find('\n', start); 
        size_t end_block = raw.rfind("```");
        
        if (end_line != string::npos && end_block != string::npos && end_block > end_line) {
            return raw.substr(end_line + 1, end_block - end_line - 1);
        }
        return raw;
    } catch (...) { return "JSON_PARSE_ERROR"; }
}

void selectLanguage() {
    cout << "\n[?] Ambiguous target. Select Language:\n";
    int i = 1; vector<string> keys;
    for(auto const& [key, val] : LANG_DB) keys.push_back(key);
    for(const auto& k : keys) { cout << i++ << ". " << LANG_DB[k].name << " "; if(i%4==0) cout<<endl;}
    cout << "\n> "; int c; cin >> c; 
    if(c>=1 && c<=keys.size()) CURRENT_LANG = LANG_DB[keys[c-1]];
    else CURRENT_LANG = LANG_DB["cpp"];
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    initLogger(); 

    if (argc < 2) {
        cout << "YORI v4.5 (Multi-File)\nUsage: yori file1 file2 ... [-o output] [-cloud/-local] [-u]" << endl;
        return 0;
    }

    vector<string> inputFiles; // v4.5: Multiple inputs
    string outputName = "";
    string mode = "local"; 
    bool explicitLang = false;
    bool dryRun = false;
    bool updateMode = false; // FLAG: Update existing source

    // Argument Parsing
    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i+1 < argc) { outputName = argv[i+1]; i++; }
        else if (arg == "-cloud") mode = "cloud";
        else if (arg == "-local") mode = "local";
        else if (arg == "-dry-run") dryRun = true;
        else if (arg == "-verbose") VERBOSE_MODE = true;
        else if (arg == "-u" || arg == "--update") updateMode = true;
        else if (arg[0] == '-') {
            string langKey = arg.substr(1);
            if (LANG_DB.count(langKey)) { CURRENT_LANG = LANG_DB[langKey]; explicitLang = true; }
        }
        else {
            inputFiles.push_back(arg); // Collect source files
        }
    }

    if (inputFiles.empty()) { cerr << "No input files." << endl; return 1; }
    if (!loadConfig(mode)) return 1;

    // Detect Lang & Output Name
    if (!explicitLang) {
        if (outputName.empty()) {
             CURRENT_LANG = LANG_DB["cpp"]; // Default to C++ if unknown
        } else {
            string ext = getExt(outputName);
            for (auto const& [key, val] : LANG_DB) {
                if (val.extension == ext) { CURRENT_LANG = val; explicitLang=true; break; }
            }
            if (!explicitLang) selectLanguage();
        }
    }

    if (outputName.empty()) outputName = stripExt(inputFiles[0]) + CURRENT_LANG.extension;
    
    // Validate Toolchain
    cout << "[CHECK] Toolchain for " << CURRENT_LANG.name << "..." << endl;
    if (execCmd(CURRENT_LANG.versionCmd).exitCode != 0) {
        cout << "   [!] Toolchain not found (" << CURRENT_LANG.versionCmd << "). Blind Mode." << endl;
    } else cout << "   [OK] Ready." << endl;

    // --- AGGREGATE SOURCES (v4.5) ---
    string aggregatedContext = "";
    vector<string> stack;
    
    for (const auto& file : inputFiles) {
        fs::path p(file);
        if (fs::exists(p)) {
            ifstream f(p);
            string raw((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
            aggregatedContext += "\n// --- START FILE: " + file + " ---\n";
            aggregatedContext += resolveImports(raw, p.parent_path(), stack);
            aggregatedContext += "\n// --- END FILE: " + file + " ---\n";
        } else {
            cerr << "Error: File not found: " << file << endl;
            return 1;
        }
    }

    // --- LOAD EXISTING CODE (Update Mode) ---
    string existingCode = "";
    if (updateMode) {
        // Construct the expected source path (e.g., if output is app.exe, look for app.cpp)
        string srcPath = stripExt(outputName) + CURRENT_LANG.extension;
        if (fs::exists(srcPath)) {
            ifstream old(srcPath);
            existingCode.assign((istreambuf_iterator<char>(old)), istreambuf_iterator<char>());
            log("INFO", "Loaded existing code for update: " + srcPath);
            cout << "   [UPDATE] Found existing source: " << srcPath << endl;
        } else {
            cout << "   [!] Update mode requested but no existing source found. Creating new." << endl;
        }
    }

    if (dryRun) { cout << "--- CONTEXT PREVIEW ---\n" << aggregatedContext << endl; return 0; }

    // --- COMPILATION LOOP ---
    string tempSrc = "temp_build" + CURRENT_LANG.extension;
    string tempBin = "temp_build.exe"; 
    string errorHistory = ""; // v4.5: Error Memory
    
    int passes = MAX_RETRIES;
    
    for(int gen=1; gen<=passes; gen++) {
        cout << "   [Pass " << gen << "] Generating " << CURRENT_LANG.name << "..." << endl;
        
        stringstream prompt;
        prompt << "ROLE: Expert " << CURRENT_LANG.name << " compiler/transpiler.\n";
        
        if (updateMode && !existingCode.empty()) {
            // MODE A: UPDATE
            prompt << "TASK: UPDATE the existing code based on the new input logic.\n";
            prompt << "CONTEXT: You have an existing codebase and new logic/files to integrate.\n";
            prompt << "REQUIREMENT: Modify [OLD CODE] to include features/fixes from [NEW INPUTS]. Output the FULL updated code.\n";
            prompt << "\n--- [OLD CODE] ---\n" << existingCode << "\n--- [END OLD CODE] ---\n";
            prompt << "\n--- [NEW INPUTS] ---\n" << aggregatedContext << "\n--- [END NEW INPUTS] ---\n";
        } else {
            // MODE B: NEW BUILD
            prompt << "TASK: Create a SINGLE, COMPLETE, RUNNABLE " << CURRENT_LANG.name << " file.\n";
            prompt << "CONTEXT: The following is a collection of source files (possibly mixed languages/pseudo-code).\n";
            prompt << "REQUIREMENT: Merge logic, resolve imports, implement main(), and output valid code only.\n";
            prompt << "\n--- INPUT SOURCES ---\n" << aggregatedContext << "\n--- END SOURCES ---\n";
        }
        
        if (!errorHistory.empty()) {
            prompt << "\n[!] PREVIOUS ERRORS (Do not repeat these mistakes):\n" << errorHistory << "\n";
            prompt << "Fix the code based on these compiler errors.\n";
        }
        
        prompt << "\nOUTPUT: Only the " << CURRENT_LANG.name << " code block.";

        string response = callAI(prompt.str());
        string code = extractCode(response);
        
        if (code.find("ERROR") == 0) { log("API_FAIL", code); continue; }

        ofstream out(tempSrc); out << code; out.close();

        // Build
        cout << "   Verifying..." << endl;
        string valCmd = CURRENT_LANG.buildCmd + " \"" + tempSrc + "\"";
        if (CURRENT_LANG.producesBinary) valCmd += " -o \"" + tempBin + "\""; // Safe quoting
        
        CmdResult build = execCmd(valCmd);
        
        if (build.exitCode == 0) {
            cout << "\nBUILD SUCCESSFUL: " << outputName << endl;
            
            if (CURRENT_LANG.producesBinary) {
                fs::copy_file(tempBin, outputName, fs::copy_options::overwrite_existing);
                cout << "   [Binary]: " << outputName << endl;
                fs::remove(tempBin);
            } else {
                fs::copy_file(tempSrc, outputName, fs::copy_options::overwrite_existing);
                cout << "   [Script]: " << outputName << endl;
            }
            fs::remove(tempSrc);
            return 0;
        } else {
            // Compilation Failed
            string err = build.output;
            cout << "   [!] Error (Line " << gen << "): " << err.substr(0, 50) << "..." << endl;
            log("FAIL", "Pass " + to_string(gen) + " failed.");
            
            // v4.5: Add to error memory for next prompt
            errorHistory += "\n--- Error Pass " + to_string(gen) + " ---\n" + err;
        }
    }

    cerr << "Failed to build after " << passes << " attempts." << endl;
    return 1;
}