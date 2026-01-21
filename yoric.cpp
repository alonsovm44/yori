/* YORI COMPILER (yori.exe) - v4.5 (Multi-File + Error Memory + Update Mode)
   Usage: yori source1.ext source2.ext [-o output] [-u] [FLAGS]
   Features: 
     - Multi-file ingestion
     - Smart Import Resolution
     - AI Error Memory
     - Configurable Toolchains
     - Update Mode (-u) for iterative dev
     - Fail-Fast on Missing Dependencies
     - AI Dependency Hinter (Interactive)
     - Pre-Flight Dependency Check (New! - Saves Tokens)
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
const int MAX_RETRIES = 15;
bool VERBOSE_MODE = false;
const string CURRENT_VERSION = "4.5.1";

// --- LOGGER SYSTEM ---
ofstream logFile;

void initLogger() {
    logFile.open("yori.log", ios::app); 
    if (logFile.is_open()) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        logFile << "\n--- SESSION START (v4.5): " << put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ---\n";
        logFile << "\n--- SESSION START (v" << CURRENT_VERSION << "): " << put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ---\n";
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
    {"rust", {"rust","Rust",".rs",  "rustc --version", "rustc", true}}, 
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

// Check if error is related to missing files/libs (Fail Fast Strategy)
bool isFatalError(const string& errMsg) {
    string lowerErr = errMsg;
    transform(lowerErr.begin(), lowerErr.end(), lowerErr.begin(), ::tolower);
    
    if (lowerErr.find("fatal error") != string::npos) return true;
    if (lowerErr.find("no such file") != string::npos) return true;
    if (lowerErr.find("file not found") != string::npos) return true;
    if (lowerErr.find("cannot open source file") != string::npos) return true;
    if (lowerErr.find("module not found") != string::npos) return true;
    if (lowerErr.find("importerror") != string::npos) return true; 
    
    return false;
}

// --- CONFIG & TOOLCHAIN OVERRIDES ---
bool loadConfig(string mode) {
    string configPath = "config.json";
    ifstream f(configPath);
    if (!f.is_open()) {
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

// --- PREPROCESSOR ---
string resolveImports(string code, fs::path basePath, vector<string>& stack) {
    stringstream ss(code);
    string line;
    string processed;
    
    while (getline(ss, line)) {
        string cleanLine = line;
        size_t first = cleanLine.find_first_not_of(" \t\r\n");
        if (first == string::npos) { processed += line + "\n"; continue; }
        cleanLine.erase(0, first);
        
        if (cleanLine.rfind("IMPORT:", 0) == 0) {
            string fname = cleanLine.substr(7);
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
                            string ext = path.extension().string();
                            processed += "\n// >>>>>> START MODULE: " + fname + " (" + ext + ") >>>>>>\n";
                            processed += nested;
                            processed += "\n// <<<<<< END MODULE: " + fname + " <<<<<<\n";
                            log("INFO", "Imported module: " + fname);
                        }
                    }
                } else {
                    // Mark as potential system/missing import for pre-check
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
        if (j.contains("error")) return "ERROR: " + j["error"].dump();
        if (PROVIDER == "local") { if (j.contains("response")) raw = j["response"]; } 
        else { if (j.contains("candidates")) raw = j["candidates"][0]["content"]["parts"][0]["text"]; }
        
        size_t start = raw.find("```"); 
        if (start == string::npos) return raw; 
        size_t end_line = raw.find('\n', start); 
        size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos && end_block > end_line) {
            return raw.substr(end_line + 1, end_block - end_line - 1);
        }
        return raw;
    } catch (...) { return "JSON_PARSE_ERROR"; }
}

void explainFatalError(const string& errorMsg) {
    cout << "\n[YORI ASSISTANT] ANALYZING fatal error..." << endl;
    string prompt = "ROLE: Helpful Tech Support.\nTASK: Fix missing file error.\nERROR: " + errorMsg.substr(0, 500) + "\nOUTPUT: Short advice.";
    string advice = callAI(prompt);
    try {
        json j = json::parse(advice);
        if (j.contains("candidates")) advice = j["candidates"][0]["content"]["parts"][0]["text"];
        else if (j.contains("response")) advice = j["response"];
    } catch(...) {}
    cout << "> Proposed solution: " << advice << endl;
}

// --- PRE-FLIGHT DEPENDENCY CHECKER --- //so it does not compile broken code and waste tokens

// Scans code for potential dependencies (includes, missing imports)
set<string> extractDependencies(const string& code) {
    set<string> deps;
    stringstream ss(code);
    string line;
    while(getline(ss, line)) {
        // Check for missing local imports identified by preprocessor
        size_t warnPos = line.find("// [WARN] IMPORT NOT FOUND: ");
        if (warnPos != string::npos) {
            deps.insert(line.substr(warnPos + 28)); // 28 = len of prefix
        }
        
        // Check for manual C/C++ includes
        size_t incPos = line.find("#include");
        if (incPos != string::npos) {
            size_t startQuote = line.find_first_of("\"<", incPos);
            size_t endQuote = line.find_first_of("\">", startQuote + 1);
            if (startQuote != string::npos && endQuote != string::npos) {
                deps.insert(line.substr(startQuote + 1, endQuote - startQuote - 1));
            }
        }
    }
    return deps;
}

bool preFlightCheck(const set<string>& deps) {
    if (deps.empty()) return true;
    
    // Only verify for compiled languages where header check is standard
    if (CURRENT_LANG.id != "cpp" && CURRENT_LANG.id != "c") return true; 

    cout << "[CHECK] Verifying dependencies locally..." << endl;
    string tempCheck = "temp_dep_check" + CURRENT_LANG.extension;
    ofstream out(tempCheck);
    
    // Create a minimal dummy file that just imports the headers
    if (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c") {
        for(const auto& d : deps) {
            // Determine if it needs quotes or angle brackets based on content, defaulting to quotes if unsure
            // But simpler: just try to include it.
            if (d.find(".h") != string::npos || d.find("/") != string::npos) out << "#include \"" << d << "\"\n";
            else out << "#include <" << d << ">\n"; 
        }
        out << "int main() { return 0; }\n";
    }
    out.close();
    
    // Compile-only flag (-c) to avoid linker errors, we just want to check headers
    string cmd = CURRENT_LANG.buildCmd + " -c \"" + tempCheck + "\""; // -c works for gcc/clang
    if (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c") {
        // cmd is constructed correctly above
    } else {
        // Fallback for others if we implement them later
        cmd = CURRENT_LANG.buildCmd + " \"" + tempCheck + "\"";
    }

    CmdResult res = execCmd(cmd);
    
    // Cleanup
    fs::remove(tempCheck);
    if (fs::exists(stripExt(tempCheck) + ".o")) fs::remove(stripExt(tempCheck) + ".o");
    if (fs::exists(stripExt(tempCheck) + ".obj")) fs::remove(stripExt(tempCheck) + ".obj");

    if (res.exitCode != 0) {
        cout << "   [!] Missing Dependency Detected!" << endl;
        // Identify which one failed from output
        for(const auto& d : deps) {
            if (res.output.find(d) != string::npos) {
                cout << "       -> " << d << " not found." << endl;
            }
        }
        // Use assistant to explain
        if (isFatalError(res.output)) {
             cout << "   [?] Analyze fatal error with AI? [y/N]: ";
             char ans; cin >> ans;
             if (ans == 'y' || ans == 'Y') explainFatalError(res.output);
        } else {
             cout << "   [ERROR LOG]:\n" << res.output.substr(0, 300) << endl;
        }
        return false;
    }
    
    cout << "   [OK] Dependencies verified." << endl;
    return true;
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
        cout << "YORI v4.5 (Multi-File)\nUsage: yori file1 ... [-o output] [-cloud/-local] [-u]" << endl;
        cout << "YORI v" << CURRENT_VERSION << " (Multi-File)\nUsage: yori file1 ... [-o output] [-cloud/-local] [-u]" << endl;
        return 0;
    }

    vector<string> inputFiles; 
    string outputName = "";
    string mode = "local"; 
    bool explicitLang = false;
    bool dryRun = false;
    bool updateMode = false; 

    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i+1 < argc) { outputName = argv[i+1]; i++; }
        else if (arg == "-cloud") mode = "cloud";
        else if (arg == "-local") mode = "local";
        else if (arg == "-dry-run") dryRun = true;
        else if (arg == "-verbose") VERBOSE_MODE = true;
        else if (arg == "-u" || arg == "--update") updateMode = true;
        else if (arg == "--version") { cout << "Yori Compiler v" << CURRENT_VERSION << endl; return 0; }
        else if (arg == "--clean") {
            cout << "[CLEAN] Removing temporary build files..." << endl;
            try {
                if (fs::exists(".yori_build.cache")) {
                    fs::remove(".yori_build.cache");
                    cout << "   Removed .yori_build.cache" << endl;
                }
                for (const auto& entry : fs::directory_iterator(fs::current_path())) {
                    if (entry.is_regular_file()) {
                        string fname = entry.path().filename().string();
                        if (fname.find("temp_build") == 0) {
                            fs::remove(entry.path());
                            cout << "   Removed " << fname << endl;
                        }
                    }
                }
            } catch (const exception& e) {
                cerr << "[ERROR] Clean failed: " << e.what() << endl;
            }
            return 0;
        }
        else if (arg == "--init") {
            cout << "[INIT] Creating project template..." << endl;
            
            if (!fs::exists("hello.yori")) {
                ofstream f("hello.yori");
                f << "// Welcome to Yori!\n";
                f << "// This is a basic template.\n\n";
                f << "PRINT(\"Hello, World!\")\n";
                f.close();
                cout << "   Created hello.yori" << endl;
            } else {
                cout << "   [SKIP] hello.yori already exists." << endl;
            }

            if (!fs::exists("config.json")) {
                ofstream f("config.json");
                f << "{\n    \"local\": {\n        \"model_id\": \"qwen2.5-coder:3b\",\n        \"api_url\": \"http://localhost:11434/api/generate\"\n    },\n    \"cloud\": {\n        \"api_key\": \"YOUR_API_KEY_HERE\",\n        \"model_id\": \"gemini-1.5-flash\"\n    },\n    \"toolchains\": {\n        \"cpp\": {\n            \"build_cmd\": \"g++ -std=c++17\"\n        }\n    }\n}\n";
                f.close();
                cout << "   Created config.json" << endl;
            } else {
                cout << "   [SKIP] config.json already exists." << endl;
            }
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            cout << "YORI Compiler v" << CURRENT_VERSION << "\n";
            cout << "Usage: yori <source_files> [options]\n\n";
            cout << "Options:\n";
            cout << "  -o <file>       Specify output filename\n";
            cout << "  -u, --update    Update mode (iterative development)\n";
            cout << "  -cloud          Use cloud AI provider (Google Gemini)\n";
            cout << "  -local          Use local AI provider (Ollama) [Default]\n";
            cout << "  -dry-run        Show preprocessed code without compiling\n";
            cout << "  -verbose        Enable verbose logging\n";
            cout << "  --version       Show version information\n";
            cout << "  --clean         Remove temporary build files\n";
            cout << "  --init          Create a new project template\n";
            cout << "  --help, -h      Show this help message\n\n";
            cout << "Supported Languages (use flag to force, e.g. -cpp):\n";
            for (const auto& [key, val] : LANG_DB) {
                cout << "  -" << left << setw(6) << key << " : " << val.name << " (" << val.extension << ")\n";
            }
            return 0;
        }
        else if (arg[0] == '-') {
            string langKey = arg.substr(1);
            if (LANG_DB.count(langKey)) { CURRENT_LANG = LANG_DB[langKey]; explicitLang = true; }
        }
        else {
            inputFiles.push_back(arg); 
        }
    }

    if (inputFiles.empty()) { cerr << "No input files." << endl; return 1; }
    if (!loadConfig(mode)) return 1;

    if (!explicitLang) {
        if (outputName.empty()) {
             CURRENT_LANG = LANG_DB["cpp"]; 
        } else {
            string ext = getExt(outputName);
            for (auto const& [key, val] : LANG_DB) {
                if (val.extension == ext) { CURRENT_LANG = val; explicitLang=true; break; }
            }
            if (!explicitLang) selectLanguage();
        }
    }

    if (outputName.empty()) outputName = stripExt(inputFiles[0]) + CURRENT_LANG.extension;
    
    cout << "[CHECK] Toolchain for " << CURRENT_LANG.name << "..." << endl;
    if (execCmd(CURRENT_LANG.versionCmd).exitCode != 0) {
        cout << "   [!] Toolchain not found (" << CURRENT_LANG.versionCmd << "). Blind Mode." << endl;
    } else cout << "   [OK] Ready." << endl;

    // --- AGGREGATE SOURCES ---
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

    // --- CACHE SYSTEM ---
    size_t currentHash = hash<string>{}(aggregatedContext + CURRENT_LANG.id + MODEL_ID + (updateMode ? "u" : "n"));
    string cacheFile = ".yori_build.cache"; 

    if (!updateMode && !dryRun && fs::exists(cacheFile) && fs::exists(outputName)) {
        ifstream cFile(cacheFile);
        size_t storedHash;
        if (cFile >> storedHash && storedHash == currentHash) {
            cout << "[CACHE] No changes detected. Using existing build." << endl;
            log("INFO", "Cache hit.");
            return 0;
        }
    }

    // --- PRE-FLIGHT CHECK (New v4.5 Feature) ---
    // Extract potential dependencies from source warnings or manual includes
    set<string> potentialDeps = extractDependencies(aggregatedContext);
    if (!preFlightCheck(potentialDeps)) {
        log("FATAL", "Pre-flight check failed.");
        return 1; // Abort before AI generation to save tokens
    }

    // --- LOAD EXISTING CODE ---
    string existingCode = "";
    if (updateMode) {
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
    string errorHistory = ""; 
    
    int passes = MAX_RETRIES;
    
    for(int gen=1; gen<=passes; gen++) {
        cout << "   [Pass " << gen << "] Generating " << CURRENT_LANG.name << "..." << endl;
        
        stringstream prompt;
        prompt << "ROLE: Expert " << CURRENT_LANG.name << " compiler/transpiler.\n";
        prompt << "STRICT RULES:\n";
        prompt << "1. DO NOT SIMULATE or MOCK missing libraries. If a library is imported, MUST include the actual header.\n";
        prompt << "2. If a dependency is missing, let the compiler fail.\n";
        
        if (updateMode && !existingCode.empty()) {
            prompt << "TASK: UPDATE existing code.\n";
            prompt << "\n--- [OLD CODE] ---\n" << existingCode << "\n--- [END OLD CODE] ---\n";
            prompt << "\n--- [NEW INPUTS] ---\n" << aggregatedContext << "\n--- [END NEW INPUTS] ---\n";
        } else {
            prompt << "TASK: Create SINGLE RUNNABLE " << CURRENT_LANG.name << " file.\n";
            prompt << "\n--- INPUT SOURCES ---\n" << aggregatedContext << "\n--- END SOURCES ---\n";
        }
        
        if (!errorHistory.empty()) {
            prompt << "\n[!] PREVIOUS ERRORS:\n" << errorHistory << "\n";
        }
        
        prompt << "\nOUTPUT: Only code.";

        string response = callAI(prompt.str());
        string code = extractCode(response);
        
        if (code.find("ERROR") == 0) { log("API_FAIL", code); continue; }

        ofstream out(tempSrc); out << code; out.close();

        cout << "   Verifying..." << endl;
        string valCmd = CURRENT_LANG.buildCmd + " \"" + tempSrc + "\"";
        if (CURRENT_LANG.producesBinary) valCmd += " -o \"" + tempBin + "\""; 
        
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

            ofstream cFile(cacheFile);
            cFile << currentHash;
            return 0;
        } else {
            string err = build.output;
            cout << "   [!] Error (Line " << gen << "): " << err.substr(0, 50) << "..." << endl;
            log("FAIL", "Pass " + to_string(gen) + " failed.");

            if (isFatalError(err)) {
                cerr << "\n[FATAL ERROR] Missing dependency/file detected. Aborting." << endl;
                cerr << "Details: " << err.substr(0, 200) << "..." << endl;
                
                cout << "   [?] Analyze fatal error with AI? [y/N]: ";
                char ans; cin >> ans;
                if (ans == 'y' || ans == 'Y') explainFatalError(err);
                
                log("FATAL", "Aborted due to missing dependencies.");
                break; 
            }
            errorHistory += "\n--- Error Pass " + to_string(gen) + " ---\n" + err;
        }
    }

    cerr << "Failed to build after " << passes << " attempts." << endl;
    return 1;
}