/* YORI COMPILER (yori.exe) - v4.4 (Robust + AI Logging)
   Usage: yori file.yori [-o output] [FLAGS]
   Features: Polyglot support, Robust File Handling, AI Debug Logging.
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

// --- LOGGER SYSTEM (NEW) ---
ofstream logFile;

void initLogger() {
    logFile.open("yori.log", ios::app); // Append mode
    if (logFile.is_open()) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        logFile << "\n--- SESSION START: " << put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ---\n";
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
    {"rust", {"rust","Rust",".rs",  "rustc --version", "rustc --crate-type bin", true}},
    {"go",   {"go",  "Go",  ".go",  "go version", "go build", true}},
    {"zig",  {"zig", "Zig", ".zig", "zig version", "zig build-exe", true}},
    {"swift",{"swift","Swift",".swift","swiftc --version", "swiftc", true}},
    {"py",   {"py",  "Python", ".py", "python --version", "python -m py_compile", false}},
    {"js",   {"js",  "JavaScript", ".js", "node --version", "node -c", false}},
    {"ts",   {"ts",  "TypeScript", ".ts", "tsc --version", "tsc --noEmit", false}},
    {"rb",   {"rb",  "Ruby", ".rb",   "ruby --version", "ruby -c", false}},
    {"php",  {"php", "PHP", ".php",   "php --version", "php -l", false}},
    {"lua",  {"lua", "Lua", ".lua",   "luac -v", "luac -p", false}},
    {"pl",   {"pl",  "Perl", ".pl",   "perl --version", "perl -c", false}},
    {"java", {"java","Java",".java","javac -version", "javac", false}}, 
    {"cs",   {"cs",  "C#",  ".cs",  "dotnet --version", "dotnet build", true}},
    {"sh",   {"sh",  "Bash", ".sh",   "bash --version", "bash -n", false}},
    {"ps1",  {"ps1", "PowerShell", ".ps1", "pwsh -v", "pwsh -Command Get-Date", false}},
    {"jl",   {"jl",  "Julia", ".jl",  "julia -v", "julia", false}},
    {"r",    {"r",   "R", ".R",       "R --version", "R CMD BATCH --no-save --no-restore", false}}, 
    {"hs",   {"hs",  "Haskell", ".hs","ghc --version", "ghc -fno-code", false}},
    {"kt",   {"kt",  "Kotlin", ".kt", "kotlinc -version", "kotlinc", false}}
};

LangProfile CURRENT_LANG; 

// --- UTILS ---
#ifdef _WIN32
#else
    #define _popen popen
    #define _pclose pclose
#endif

string getExePath() {
    char buffer[1024] = {0};
#ifdef _WIN32
    if (GetModuleFileNameA(NULL, buffer, 1024) == 0) return "";
#else
    ssize_t count = readlink("/proc/self/exe", buffer, 1023);
    if (count != -1) buffer[count] = '\0'; else return "";
#endif
    return string(buffer);
}

struct CmdResult { string output; int exitCode; };

CmdResult execCmd(string cmd) {
    array<char, 128> buffer; string result; string full_cmd = cmd + " 2>&1"; 
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

// --- CONFIG ---
bool loadConfig(string mode) {
    string configPath = "config.json";
    string exeStr = getExePath();
    if (!exeStr.empty()) {
        fs::path exePath(exeStr);
        fs::path installConfig = exePath.parent_path().parent_path() / "config.json";
        if (fs::exists(installConfig)) configPath = installConfig.string();
        else if (fs::exists("C:\\Yori\\config.json")) configPath = "C:\\Yori\\config.json";
    }

    ifstream f(configPath);
    if (!f.is_open() && configPath != "config.json") f.open("config.json");

    if (!f.is_open()) { cerr << "FATAL: config.json missing." << endl; return false; }
    try {
        json j = json::parse(f);
        if (!j.contains(mode)) return false;
        json profile = j[mode];
        PROVIDER = mode;
        if (mode == "cloud") API_KEY = profile["api_key"];
        MODEL_ID = profile["model_id"];
        if (mode == "local") API_URL = profile.contains("api_url") ? profile["api_url"].get<string>() : "http://localhost:11434/api/generate";
        
        log("CONFIG", "Loaded profile: " + mode + " (" + MODEL_ID + ")");
        return true;
    } catch (...) { return false; }
}

// --- PREPROCESSOR ---
string resolveImports(string code, string basePath) {
    stringstream ss(code);
    string line;
    string processed;
    while (getline(ss, line)) {
        string cleanLine = line;
        cleanLine.erase(0, cleanLine.find_first_not_of(" \t\r\n"));
        
        bool isImport = (cleanLine.rfind("IMPORT:", 0) == 0);
        bool isInclude = (cleanLine.rfind("INCLUDE:", 0) == 0);

        if (isImport || isInclude) {
            string fname = cleanLine.substr(isImport ? 7 : 8);
            fname.erase(0, fname.find_first_not_of(" \t\r\n\"'"));
            fname.erase(fname.find_last_not_of(" \t\r\n\"'") + 1);
            
            fs::path path = basePath;
            if (basePath.empty()) path = fname; else path /= fname;
            
            if (fs::exists(path)) {
                ifstream imp(path);
                if (imp.is_open()) {
                    string content((istreambuf_iterator<char>(imp)), istreambuf_iterator<char>());
                    processed += "\n// --- IMPORT: " + fname + " ---\n" + content + "\n// --- END IMPORT ---\n";
                    log("INFO", "Imported module: " + fname);
                }
            } else log("WARN", "Missing import: " + fname);
        } else processed += line + "\n";
    }
    return processed;
}

// --- AI CORE ---
string callAI(string prompt) {
    string response;
    while (true) {
        json body; string url;
        if (PROVIDER == "local") { body["model"]=MODEL_ID; body["prompt"]=prompt; body["stream"]=false; url=API_URL; }
        else { body["contents"][0]["parts"][0]["text"]=prompt; url="https://generativelanguage.googleapis.com/v1beta/models/"+MODEL_ID+":generateContent?key="+API_KEY; }
        ofstream file("request_temp.json"); file << body.dump(-1, ' ', false, json::error_handler_t::replace); file.close();
        string cmd = "curl -s -X POST -H \"Content-Type: application/json\" -d @request_temp.json \"" + url + "\"";
        
        CmdResult res = execCmd(cmd);
        response = res.output;
        
        remove("request_temp.json");
        if (PROVIDER == "cloud" && response.find("429") != string::npos) { 
            log("WARN", "Rate limit hit. Waiting 30s...");
            this_thread::sleep_for(chrono::seconds(30)); continue; 
        }
        break;
    }
    return response;
}

string extractCode(string jsonResponse) {
    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        if (j.contains("error")) {
            string msg = "API Error";
            if (j["error"].is_object() && j["error"].contains("message")) msg = j["error"]["message"];
            log("ERROR", "AI API returned: " + msg);
            return "ERROR: " + msg;
        }
        if (PROVIDER == "local") { if (j.contains("response")) raw = j["response"]; } 
        else { if (j.contains("candidates")) raw = j["candidates"][0]["content"]["parts"][0]["text"]; }
        
        size_t start = raw.find("```"); if (start == string::npos) return raw;
        size_t end_line = raw.find('\n', start); size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos) return raw.substr(end_line + 1, end_block - end_line - 1);
        return raw;
    } catch (exception& e) { 
        log("ERROR", "JSON Parse Failed: " + string(e.what()));
        return "JSON_PARSE_ERROR"; 
    }
}

void selectLanguage() {
    cout << "\n[?] Ambiguous output. Please select target language:\n" << endl;
    vector<string> keys;
    for(auto const& [key, val] : LANG_DB) keys.push_back(key);
    int i = 1;
    for (const auto& key : keys) {
        cout << std::left << std::setw(4) << i << std::setw(15) << (LANG_DB[key].name + " (" + LANG_DB[key].id + ")");
        if (i % 3 == 0) cout << endl;
        i++;
    }
    cout << "\n\n> Selection [1-" << keys.size() << "]: ";
    int choice; if (!(cin >> choice)) choice = 1;
    if (choice < 1 || choice > keys.size()) choice = 1;
    CURRENT_LANG = LANG_DB[keys[choice-1]];
    string dummy; getline(cin, dummy); 
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    initLogger(); // Start logging session

    if (argc < 2) {
        cout << "Usage: yori <input.extension> [-o output.extension] [-cloud | -local] [-u]\nNote: Yori accepts any text file as input, it does not need to be a .yori file" << endl;
        return 0;
    }

    string inputFile = "";
    string outputName = "";
    string mode = "local"; 
    bool explicitLang = false;
    bool updateMode = false;

    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if (arg == "--version" || arg == "-v") {
            cout << "Yori Compiler v4.4" << endl;
            return 0;
        }
        else if (arg == "-cloud") mode = "cloud";
        else if (arg == "-local") mode = "local";
        else if (arg == "-u") updateMode = true;
        else if (arg == "-verbose") VERBOSE_MODE = true;
        else if (arg == "-o" && i+1 < argc) { outputName = argv[i+1]; i++; }
        else if (arg[0] == '-') {
            string cleanArg = arg.substr(1); 
            if (LANG_DB.count(cleanArg)) { CURRENT_LANG = LANG_DB[cleanArg]; explicitLang = true; }
        }
        else if (inputFile.empty()) inputFile = arg;
    }

    if (inputFile.empty()) { cerr << "Error: Input missing." << endl; return 1; }

    if (!loadConfig(mode)) return 1;

    if (!explicitLang) {
        if (outputName.empty()) {
             cout << "[INFO] No lang specified. Defaulting to C++." << endl;
             CURRENT_LANG = LANG_DB["cpp"];
        } else {
            string ext = getExt(outputName);
            bool found = false;
            for (auto const& [key, val] : LANG_DB) {
                if (val.extension == ext) { CURRENT_LANG = val; found = true; break; }
            }
            if (!found) selectLanguage();
            else cout << "[INFO] Detected: " << CURRENT_LANG.name << endl;
        }
    }

    if (outputName.empty()) outputName = stripExt(inputFile) + CURRENT_LANG.extension;
    log("INFO", "Target: " + CURRENT_LANG.name + " | Output: " + outputName);

    // HEALTH CHECK
    cout << "[CHECK] Looking for " << CURRENT_LANG.name << " toolchain..." << endl;
    CmdResult check = execCmd(CURRENT_LANG.versionCmd);
    bool toolchainActive = true;

    if (check.exitCode != 0) {
        cerr << "\n[MISSING TOOLS] '" << CURRENT_LANG.versionCmd << "' returned code " << check.exitCode << "." << endl;
        log("WARN", "Toolchain missing for " + CURRENT_LANG.name);
        cout << "Continue in BLIND MODE? [y/N]: ";
        char ans; cin >> ans; if (ans != 'y' && ans != 'Y') return 1;
        toolchainActive = false;
    } else {
        cout << "   [OK] Found." << endl;
    }

    ifstream f(inputFile);
    if (!f.is_open()) { cerr << "Error: Input missing." << endl; return 1; }
    string rawCode((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    fs::path p(inputFile);
    string finalLogic = resolveImports(rawCode, p.parent_path().string());

    // --- CACHING ---
    size_t currentHash = hash<string>{}(finalLogic + CURRENT_LANG.id + to_string(updateMode) + MODEL_ID);
    string cacheFile = inputFile + ".cache";
    
    if (!updateMode && fs::exists(cacheFile) && fs::exists(outputName)) {
        ifstream cFile(cacheFile);
        size_t storedHash;
        if (cFile >> storedHash && storedHash == currentHash) {
            cout << "[CACHE] No changes detected. Using existing build." << endl;
            log("INFO", "Cache hit for " + inputFile);
            return 0;
        }
    }

    string existingCode = "";
    if (updateMode) {
        string readPath = outputName;
        bool safeToRead = true;
        if (CURRENT_LANG.producesBinary) {
            string srcPath = stripExt(outputName) + CURRENT_LANG.extension;
            if (fs::exists(srcPath)) readPath = srcPath;
            else safeToRead = false;
        }
        if (safeToRead) {
            ifstream old(readPath);
            if (old.is_open()) {
                string content((istreambuf_iterator<char>(old)), istreambuf_iterator<char>());
                if (content.find('\0') == string::npos) existingCode = content;
                else log("WARN", "Ignored binary file for update: " + readPath);
            }
        }
    }

    string tempSrc = "temp_src" + CURRENT_LANG.extension;
    string tempBin = "temp_bin.exe"; 
    string currentError = "";
    int passes = toolchainActive ? MAX_RETRIES : 1;

    for(int gen=1; gen<=passes; gen++) {
        cout << "   [Pass " << gen << "] Writing " << CURRENT_LANG.name << "..." << endl;
        log("GEN " + to_string(gen), "Generating code...");
        
        string prompt;
        if (currentError.empty()) {
            if (updateMode && !existingCode.empty()) prompt = "ROLE: Expert " + CURRENT_LANG.name + " Dev.\nTASK: Update code.\n[OLD CODE]:\n" + existingCode + "\n[CHANGES]:\n" + finalLogic + "\nOUTPUT: Full " + CURRENT_LANG.name + " code.";
            else prompt = "ROLE: Expert " + CURRENT_LANG.name + " Dev.\nTASK: Write single-file program.\nINSTRUCTIONS:\n" + finalLogic + "\nOUTPUT: Valid " + CURRENT_LANG.name + " code only.";
        } else {
            prompt = "ROLE: Expert Debugger.\nTASK: Fix " + CURRENT_LANG.name + " error.\nERROR:\n" + currentError + "\nCODE:\n" + finalLogic + "\nOUTPUT: Corrected code.";
            log("EVOLUTION", "Fixing error: " + currentError.substr(0, 100) + "...");
        }

        if (VERBOSE_MODE) cout << "\n[DEBUG] Prompt sent to AI:\n" << prompt << "\n" << endl;

        string response = callAI(prompt);
        string code = extractCode(response);
        if (code.find("ERROR:") == 0) { cerr << "AI Error: " << code << endl; return 1; }
        
        ofstream out(tempSrc); out << code; out.close();

        if (!toolchainActive) {
            fs::copy_file(tempSrc, outputName, fs::copy_options::overwrite_existing);
            cout << "\nSaved (Blind): " << outputName << endl;
            log("SUCCESS", "Blind save completed.");
            return 0;
        }

        cout << "   Verifying..." << endl;
        string valCmd = CURRENT_LANG.buildCmd + " " + tempSrc; 
        
        if (CURRENT_LANG.producesBinary) {
             if (fs::exists(tempBin)) fs::remove(tempBin);
             if (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c" || CURRENT_LANG.id == "go" || CURRENT_LANG.id == "rust") {
                 if (CURRENT_LANG.id == "rust") valCmd += " -o " + tempBin;
                 else valCmd += " -o " + tempBin;
             }
        }

        CmdResult buildResult = execCmd(valCmd);
        bool success = (buildResult.exitCode == 0);

        if (success && CURRENT_LANG.producesBinary) {
            if (!fs::exists(tempBin) || fs::file_size(tempBin) == 0) {
                success = false;
                currentError = "Compiler returned success but binary missing/empty.";
                log("FAIL", "Ghost binary detected (0KB).");
            }
        } else if (!success) {
            currentError = buildResult.output;
            log("FAIL", "Compilation failed. Exit code: " + to_string(buildResult.exitCode));
            if (VERBOSE_MODE) cout << "\n[DEBUG] Compiler Output:\n" << currentError << "\n" << endl;
        }

        if (success) {
            cout << "\nBUILD SUCCESSFUL: " << outputName << endl;
            log("SUCCESS", "Build valid on Pass " + to_string(gen));
            
            try {
                if (CURRENT_LANG.producesBinary) {
                    if (fs::exists(outputName)) fs::remove(outputName);
                    fs::copy_file(tempBin, outputName);
                    cout << "   [Binary]: " << outputName << endl;
                    
                    string srcName = stripExt(outputName) + CURRENT_LANG.extension;
                    if (fs::exists(srcName)) fs::remove(srcName);
                    fs::copy_file(tempSrc, srcName);
                    cout << "   [Source]: " << srcName << endl;
                } else {
                    if (fs::exists(outputName)) fs::remove(outputName);
                    fs::copy_file(tempSrc, outputName);
                    cout << "   [Script]: " << outputName << endl;
                }
            } catch (fs::filesystem_error& e) {
                cerr << "FATAL FILE ERROR: " << e.what() << endl;
                log("FATAL", "File move failed: " + string(e.what()));
                return 1;
            }

            if (fs::exists("temp_check.exe")) fs::remove("temp_check.exe");
            if (fs::exists(tempBin)) fs::remove(tempBin);
            if (fs::exists(tempSrc)) fs::remove(tempSrc);
            
            // Save Cache
            ofstream cFile(cacheFile);
            cFile << currentHash;

            return 0;
        }
    }

    cerr << "\nFAILED to generate valid code." << endl;
    log("FATAL", "Max generations reached. Aborting.");
    return 1;
}