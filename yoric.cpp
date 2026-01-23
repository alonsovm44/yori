/* YORI COMPILER (yori.exe) - v4.9.4 (Diff Command Added)
   Usage: yori source1.ext source2.ext [-o output] [-u] [FLAGS]
   Features: 
     - Multi-file ingestion
     - Smart Import Resolution
     - AI Error Memory
     - Configurable Toolchains
     - Update Mode (-u) for iterative dev
     - Fail-Fast on Missing Dependencies
     - AI Dependency Hinter (Interactive)
     - Pre-Flight Dependency Check
     - CLI Config Management
     - Modular Provider System (Groq, DeepSeek, OpenAI)
     - Auto-Discovery of Local Ollama Models
     - Smart Binary vs Source Output Detection
     - Explain Command (Auto-Documentation with Language Support)
     - Fix Command (Natural Language Repair)
     - Diff Command (Semantic Change Analysis)
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
string PROTOCOL = "ollama"; // 'google', 'openai', 'ollama'
string API_KEY = "";
string MODEL_ID = ""; 
string API_URL = "";
const int MAX_RETRIES = 15;
bool VERBOSE_MODE = false;
const string CURRENT_VERSION = "4.9.4"; 

// --- LOGGER SYSTEM ---
ofstream logFile;

void initLogger() {
    logFile.open("yori.log", ios::app); 
    if (logFile.is_open()) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
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

// --- UTILS DECLARATION ---
struct CmdResult { string output; int exitCode; };

CmdResult execCmd(string cmd) {
    array<char, 128> buffer; string result; 
    string full_cmd = cmd + " 2>&1"; 
    
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
    {"java", {"java","Java",".java","javac -version", "javac", false}},
    {"php",  {"php", "PHP", ".php", "php -v", "php -l", false}},
    {"rb",   {"rb",  "Ruby", ".rb", "ruby -v", "ruby -c", false}},
    {"lua",  {"lua", "Lua", ".lua", "lua -v", "luac -p", false}},
    {"pl",   {"pl",  "Perl", ".pl", "perl -v", "perl -c", false}},
    {"sh",   {"sh",  "Bash", ".sh", "bash --version", "bash -n", false}},
    {"swift",{"swift","Swift",".swift","swift --version", "swiftc", true}},
    {"kt",   {"kt",  "Kotlin", ".kt", "kotlinc -version", "kotlinc", false}},
    {"scala",{"scala","Scala",".scala","scala -version", "scalac", false}},
    {"hs",   {"hs",  "Haskell", ".hs", "ghc --version", "ghc", true}},
    {"jl",   {"jl",  "Julia", ".jl", "julia --version", "julia", false}},
    {"dart", {"dart","Dart",".dart","dart --version", "dart compile exe", true}},
    {"zig",  {"zig", "Zig", ".zig", "zig version", "zig build-exe", true}},
    {"nim",  {"nim", "Nim", ".nim", "nim --version", "nim c", true}},
    {"r",    {"r",   "R",   ".r",   "R --version", "Rscript", false}}
};

LangProfile CURRENT_LANG; 

// --- CONFIG & TOOLCHAIN OVERRIDES ---
bool loadConfig(string mode) {
    string configPath = "config.json";
    ifstream f(configPath);
    if (!f.is_open()) {
        if(mode == "local") {
            API_URL = "http://localhost:11434/api/generate";
            PROTOCOL = "ollama";
        } else {
            PROTOCOL = "google"; 
        }
        return true; 
    }

    try {
        json j = json::parse(f);
        if (j.contains(mode)) {
            json profile = j[mode];
            PROVIDER = mode;
            
            MODEL_ID = profile.value("model_id", "gemini-pro");
            
            if (profile.contains("protocol")) PROTOCOL = profile["protocol"];
            else PROTOCOL = (mode == "cloud") ? "google" : "ollama"; 

            if (profile.contains("api_url")) {
                API_URL = profile["api_url"];
            } else {
                if (PROTOCOL == "google") API_URL = "https://generativelanguage.googleapis.com/v1beta/models/" + MODEL_ID + ":generateContent";
                else if (PROTOCOL == "openai") API_URL = "https://api.openai.com/v1/chat/completions";
                else if (mode == "local") API_URL = "http://localhost:11434/api/generate";
            }

            if (mode == "cloud") API_KEY = profile.value("api_key", "");
        }
        if (j.contains("toolchains")) {
            for (auto& [key, val] : j["toolchains"].items()) {
                if (LANG_DB.count(key)) {
                    if (val.contains("build_cmd")) LANG_DB[key].buildCmd = val["build_cmd"];
                    if (val.contains("version_cmd")) LANG_DB[key].versionCmd = val["version_cmd"];
                }
            }
        }
        return true;
    } catch (...) { return false; }
}

// --- AI CORE ---
string callAI(string prompt) {
    string response;
    string url = API_URL;
    
    json body;
    string extraHeaders = "";

    if (PROTOCOL == "google") {
        body["contents"][0]["parts"][0]["text"] = prompt;
        if (url.find("?key=") == string::npos) url += "?key=" + API_KEY;
    } 
    else if (PROTOCOL == "openai") {
        body["model"] = MODEL_ID;
        body["messages"][0]["role"] = "user";
        body["messages"][0]["content"] = prompt;
        extraHeaders = " -H \"Authorization: Bearer " + API_KEY + "\"";
    }
    else { 
        body["model"] = MODEL_ID;
        body["prompt"] = prompt;
        body["stream"] = false; 
    }

    for(int i=0; i<3; i++) {
        ofstream file("request_temp.json"); 
        file << body.dump(-1, ' ', false, json::error_handler_t::replace); 
        file.close();
        
        string verbosity = VERBOSE_MODE ? " -v" : " -s";
        string cmd = "curl" + verbosity + " -X POST -H \"Content-Type: application/json\"" + extraHeaders + " -d @request_temp.json \"" + url + "\"";
        
        CmdResult res = execCmd(cmd);
        response = res.output;
        remove("request_temp.json");
        
        if (VERBOSE_MODE) cout << "\n[DEBUG] Raw Response: " << response << endl;

        if (response.find("401 Unauthorized") != string::npos) return "ERROR: 401 Unauthorized (Check API Key)";
        if (response.find("404 Not Found") != string::npos) return "ERROR: 404 Not Found (Check URL)";

        if (PROTOCOL == "google" && response.find("429") != string::npos) { 
             log("WARN", "API 429 Rate Limit. Backoff...");
             this_thread::sleep_for(chrono::seconds(5 * (i+1)));
             continue; 
        }
        break;
    }
    return response;
}

string extractCode(string jsonResponse) {
    if (jsonResponse.empty()) return "ERROR: Empty response from API";
    if (jsonResponse.find("ERROR:") == 0) return jsonResponse;

    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        
        if (j.contains("error")) {
            if (j["error"].is_object() && j["error"].contains("message")) 
                return "ERROR: API Error - " + j["error"]["message"].get<string>();
            return "ERROR: " + j["error"].dump();
        }
        
        if (j.contains("choices") && j["choices"].size() > 0) {
            if (j["choices"][0].contains("message"))
                raw = j["choices"][0]["message"]["content"];
            else if (j["choices"][0].contains("text"))
                raw = j["choices"][0]["text"];
        }
        else if (j.contains("candidates") && j["candidates"].size() > 0) {
             raw = j["candidates"][0]["content"]["parts"][0]["text"];
        }
        else if (j.contains("response")) {
            raw = j["response"];
        } 
        else {
            return "ERROR: UNKNOWN_RESPONSE_FORMAT: " + jsonResponse.substr(0, 100);
        }
        
        size_t start = raw.find("```"); 
        if (start == string::npos) return raw; 
        size_t end_line = raw.find('\n', start); 
        size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos && end_block > end_line) {
            return raw.substr(end_line + 1, end_block - end_line - 1);
        }
        return raw;
    } catch (const exception& e) { 
        string safeMsg = jsonResponse;
        if (safeMsg.length() > 200) safeMsg = safeMsg.substr(0, 200) + "...";
        replace(safeMsg.begin(), safeMsg.end(), '\n', ' ');
        return "ERROR: JSON Parsing Failed. Response was: " + safeMsg; 
    }
}

void explainFatalError(const string& errorMsg) {
    cout << "\n[YORI ASSISTANT] ANALYZING fatal error..." << endl;
    string prompt = "ROLE: Helpful Tech Support.\nTASK: Fix missing file error.\nERROR: " + errorMsg.substr(0, 500) + "\nOUTPUT: Short advice.";
    string advice = callAI(prompt);
    
    try {
        json j = json::parse(advice);
        if (j.contains("candidates")) advice = j["candidates"][0]["content"]["parts"][0]["text"];
        else if (j.contains("response")) advice = j["response"];
        else if (j.contains("choices")) advice = j["choices"][0]["message"]["content"];
    } catch(...) {}
    cout << "> Proposed solution: " << advice << endl;
}

// --- CONFIG COMMANDS ---
void updateConfigFile(string key, string value) {
    string configPath = "config.json";
    json j;
    
    if (fs::exists(configPath)) {
        ifstream i(configPath);
        if (i.is_open()) { try { i >> j; } catch(...) {} }
    }
    
    if (!j.contains("cloud")) j["cloud"] = json::object();
    if (!j.contains("local")) j["local"] = json::object();

    if (key == "api-key") {
        j["cloud"]["api_key"] = value;
        cout << "[CONFIG] Updated cloud.api_key." << endl;
    } else if (key == "model-cloud") {
         j["cloud"]["model_id"] = value;
         cout << "[CONFIG] Updated cloud.model_id to " << value << endl;
    } else if (key == "model-local") {
         j["local"]["model_id"] = value;
         cout << "[CONFIG] Updated local.model_id to " << value << endl;
    } else if (key == "url-local") {
         j["local"]["api_url"] = value;
         cout << "[CONFIG] Updated local.api_url to " << value << endl;
    } else if (key == "url-cloud") { 
         j["cloud"]["api_url"] = value;
         cout << "[CONFIG] Updated cloud.api_url to " << value << endl;
    } else if (key == "cloud-protocol") { 
         if (value != "google" && value != "openai") {
             cout << "[ERROR] Protocol must be 'google' or 'openai'." << endl; return;
         }
         j["cloud"]["protocol"] = value;
         cout << "[CONFIG] Updated cloud.protocol to " << value << endl;
    } else {
        cout << "[ERROR] Unknown config key." << endl;
        return;
    }

    ofstream o(configPath);
    o << j.dump(4);
    cout << "[SUCCESS] Saved to " << configPath << endl;
}

void selectOllamaModel() {
    cout << "[INFO] Scanning for local Ollama models..." << endl;
    string url = "http://localhost:11434/api/tags";
    
    string configPath = "config.json";
    if (fs::exists(configPath)) {
        try {
            ifstream i(configPath); json j; i >> j;
            if (j.contains("local") && j["local"].contains("api_url")) {
                string currentUrl = j["local"]["api_url"];
                size_t pos = currentUrl.find("/api/");
                if (pos != string::npos) url = currentUrl.substr(0, pos) + "/api/tags";
            }
        } catch(...) {}
    }

    string cmd = "curl -s \"" + url + "\"";
    CmdResult res = execCmd(cmd);

    if (res.exitCode != 0 || res.output.empty()) {
        cout << "[ERROR] Could not connect to Ollama at " << url << endl;
        cout << "Make sure Ollama is running." << endl;
        return;
    }

    try {
        json j = json::parse(res.output);
        if (!j.contains("models")) { cout << "[ERROR] Unexpected response format." << endl; return; }

        vector<string> models;
        cout << "\n--- Installed Local Models ---\n";
        int idx = 1;
        for (auto& m : j["models"]) {
            string name = m["name"];
            models.push_back(name);
            cout << idx++ << ". " << name << endl;
        }

        if (models.empty()) { cout << "[WARN] No models found." << endl; return; }

        cout << "\nSelect model number (or 0 to cancel): ";
        int choice;
        if (cin >> choice && choice > 0 && choice <= models.size()) {
            updateConfigFile("model-local", models[choice-1]);
        } 
    } catch (...) { cout << "[ERROR] JSON Parsing failed." << endl; }
}

void openApiKeyPage() {
    cout << "[INFO] Opening Google AI Studio..." << endl;
    #ifdef _WIN32
    system("start https://aistudio.google.com/app/apikey");
    #else
    system("xdg-open https://aistudio.google.com/app/apikey");
    #endif
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
                    processed += "// [WARN] IMPORT NOT FOUND: " + fname + "\n";
                }
            } catch (...) { processed += "// [ERROR] PATH EXCEPTION\n"; }
        } else {
            processed += line + "\n";
        }
    }
    return processed;
}

set<string> extractDependencies(const string& code) {
    set<string> deps;
    stringstream ss(code);
    string line;
    while(getline(ss, line)) {
        size_t warnPos = line.find("// [WARN] IMPORT NOT FOUND: ");
        if (warnPos != string::npos) {
            deps.insert(line.substr(warnPos + 28)); 
        }
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
    if (CURRENT_LANG.id != "cpp" && CURRENT_LANG.id != "c") return true; 

    cout << "[CHECK] Verifying dependencies locally..." << endl;
    string tempCheck = "temp_dep_check" + CURRENT_LANG.extension;
    ofstream out(tempCheck);
    
    if (CURRENT_LANG.id == "cpp" || CURRENT_LANG.id == "c") {
        for(const auto& d : deps) {
            if (d.find(".h") != string::npos || d.find("/") != string::npos) out << "#include \"" << d << "\"\n";
            else out << "#include <" << d << ">\n"; 
        }
        out << "int main() { return 0; }\n";
    }
    out.close();
    
    string cmd = CURRENT_LANG.buildCmd + " -c \"" + tempCheck + "\""; 
    CmdResult res = execCmd(cmd);
    
    fs::remove(tempCheck);
    if (fs::exists(stripExt(tempCheck) + ".o")) fs::remove(stripExt(tempCheck) + ".o");
    if (fs::exists(stripExt(tempCheck) + ".obj")) fs::remove(stripExt(tempCheck) + ".obj");

    if (res.exitCode != 0) {
        cout << "   [!] Missing Dependency Detected!" << endl;
        for(const auto& d : deps) {
            if (res.output.find(d) != string::npos) {
                cout << "       -> " << d << " not found." << endl;
            }
        }
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
    auto startTime = std::chrono::high_resolution_clock::now();
    initLogger(); 

    if (argc < 2) {
        cout << "YORI v" << CURRENT_VERSION << " (Multi-File)\nUsage: yori file1 ... [-o output] [-cloud/-local] [-u]" << endl;
        cout << "Commands:\n  config <key> <val> : Update config.json\n  config model-local : Detect installed Ollama models\n";
        cout << "  fix <file> \"desc\"  : AI-powered code repair\n";
        cout << "  explain <file> [lg] : Generate commented documentation\n";
        cout << "  diff <f1> <f2> [lg] : Generate semantic diff report\n";
        return 0;
    }

    // --- COMMAND MODE HANDLING ---
    string cmd = argv[1];
    
    // CONFIG COMMAND
    if (cmd == "config") {
        if (argc < 3) {
            cout << "Usage: yori config <key> <value>\n";
            cout << "       yori config model-local (Interactive selection)\n\n";
            cout << "Keys:\n";
            cout << "  api-key         : Set Cloud API Key\n";
            cout << "  cloud-protocol  : Set protocol ('openai', 'google', 'ollama')\n";
            cout << "  model-cloud     : Set Cloud Model ID\n";
            cout << "  url-cloud       : Set Cloud API URL\n";
            cout << "  model-local     : Set Local Model ID\n";
            cout << "  url-local       : Set Local API URL\n";
            return 1;
        }
        string key = argv[2];
        if (key == "model-local" && argc == 3) {
            selectOllamaModel();
            return 0;
        }
        if (argc < 4) {
             cout << "[ERROR] Missing value for key: " << key << endl;
             return 1;
        }
        updateConfigFile(key, argv[3]);
        return 0;
    }
    
    // UTILS COMMANDS
    if (cmd == "get-key" || cmd == "new-key") {
        openApiKeyPage();
        return 0;
    }

    // FIX COMMAND
    if (cmd == "fix") {
        if (argc < 4) {
            cout << "Usage: yori fix <file> \"instruction\" [-cloud/-local]" << endl;
            return 1;
        }
        string targetFile = argv[2];
        string instruction = argv[3];
        string mode = "local"; 

        for(int i=4; i<argc; i++) {
            string arg = argv[i];
            if (arg == "-cloud") mode = "cloud";
            else if (arg == "-local") mode = "local";
        }

        if (!loadConfig(mode)) return 1;

        if (!fs::exists(targetFile)) {
            cout << "[ERROR] File not found: " << targetFile << endl;
            return 1;
        }

        cout << "[FIX] Reading " << targetFile << "..." << endl;
        ifstream f(targetFile);
        string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        f.close();

        string ext = getExt(targetFile);
        string langName = "Code";
        for(auto const& [key, val] : LANG_DB) {
            if(val.extension == ext) { langName = val.name; break; }
        }

        cout << "[AI] Applying fix (" << mode << ")..." << endl;
        stringstream prompt;
        prompt << "ROLE: Expert " << langName << " developer.\n";
        prompt << "TASK: Fix the code based on the instruction.\n";
        prompt << "INSTRUCTION: " << instruction << "\n";
        prompt << "CODE:\n" << content << "\n";
        prompt << "OUTPUT: Return ONLY the fixed code. No markdown. No explanations.";

        string response = callAI(prompt.str());
        string fixedCode = extractCode(response);

        if (fixedCode.find("ERROR:") == 0) {
            cout << "   [!] API Error: " << fixedCode.substr(6) << endl;
            return 1;
        }

        ofstream out(targetFile);
        out << fixedCode;
        out.close();
        
        cout << "[SUCCESS] File updated: " << targetFile << endl;
        return 0;
    }

    // EXPLAIN COMMAND (Modified with Language Support)
    if (cmd == "explain") {
        if (argc < 3) {
            cout << "Usage: yori explain <file> [-cloud/-local] [language]" << endl;
            return 1;
        }
        string targetFile = argv[2];
        string mode = "local"; 
        string language = "English"; // Default

        for(int i=3; i<argc; i++) {
            string arg = argv[i];
            if (arg == "-cloud") mode = "cloud";
            else if (arg == "-local") mode = "local";
            else language = arg;
        }

        if (!loadConfig(mode)) return 1;

        if (!fs::exists(targetFile)) {
            cout << "[ERROR] File not found: " << targetFile << endl;
            return 1;
        }

        cout << "[EXPLAIN] Reading " << targetFile << "..." << endl;
        ifstream f(targetFile);
        string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        f.close();

        cout << "[AI] Generating documentation in " << language << " (" << mode << ")..." << endl;
        stringstream prompt;
        prompt << "TASK: Add high-quality technical documentation comments to the provided code in " << language << ".\n";
        prompt << "STRICT RULES:\n";
        prompt << "1. RETURN THE FULL SOURCE CODE exactly as provided but with added comments.\n";
        prompt << "2. DO NOT simplify the code. DO NOT replace it with examples like 'Hello World'.\n";
        prompt << "3. Use the language's standard comment syntax.\n";
        prompt << "4. Document functions, logic blocks, and variables.\n";
        prompt << "5. Ensure all comments are written in " << language << ".\n";
        prompt << "6. Return ONLY the code in a markdown block.\n\n";
        prompt << "CODE TO DOCUMENT:\n" << content;

        string response = callAI(prompt.str());
        string docCode = extractCode(response);

        if (docCode.find("ERROR:") == 0) {
            cout << "   [!] API Error: " << docCode.substr(6) << endl;
            return 1;
        }

        string ext = getExt(targetFile);
        string docFile = stripExt(targetFile) + "_doc" + ext;
        
        ofstream out(docFile);
        out << docCode;
        out.close();
        
        cout << "[SUCCESS] Documentation generated: " << docFile << endl;
        return 0;
    }

    // DIFF COMMAND (New!)
    if (cmd == "diff") {
        if (argc < 4) {
            cout << "Usage: yori diff <fileA> <fileB> [-cloud/-local] [language]" << endl;
            return 1;
        }
        string fileA = argv[2];
        string fileB = argv[3];
        string mode = "local";
        string language = "English";

        for(int i=4; i<argc; i++) {
            string arg = argv[i];
            if (arg == "-cloud") mode = "cloud";
            else if (arg == "-local") mode = "local";
            else language = arg;
        }

        if (!loadConfig(mode)) return 1;

        if (!fs::exists(fileA) || !fs::exists(fileB)) {
            cout << "[ERROR] One or both files not found." << endl;
            return 1;
        }

        cout << "[DIFF] Comparing " << fileA << " vs " << fileB << "..." << endl;
        
        ifstream fa(fileA), fb(fileB);
        string contentA((istreambuf_iterator<char>(fa)), istreambuf_iterator<char>());
        string contentB((istreambuf_iterator<char>(fb)), istreambuf_iterator<char>());
        fa.close(); fb.close();

        stringstream prompt;
        prompt << "ROLE: Expert Software Auditor.\n";
        prompt << "TASK: Compare two source files and generate a semantic diff report in " << language << ".\n";
        prompt << "REPORT FORMAT:\n";
        prompt << "1. Brief Summary of changes.\n";
        prompt << "2. Changed Functions (What changed and where).\n";
        prompt << "3. Additional Observations (Potential bugs, improvements).\n";
        prompt << "RETURN: Only the report in Markdown format.\n\n";
        prompt << "--- FILE A (" << fileA << ") ---\n" << contentA << "\n";
        prompt << "\n--- FILE B (" << fileB << ") ---\n" << contentB << "\n";

        cout << "[AI] Analyzing changes (" << mode << ")..." << endl;
        string res = callAI(prompt.str());
        // For diff, we don't strict extract code blocks as the output IS the report (text)
        // But some models might wrap MD in blocks. Check briefly.
        string report = res;
        try {
            json j = json::parse(res);
            if (j.contains("choices")) report = j["choices"][0]["message"]["content"];
            else if (j.contains("candidates")) report = j["candidates"][0]["content"]["parts"][0]["text"];
            else if (j.contains("response")) report = j["response"];
        } catch(...) {}

        string outName = fs::path(fileA).stem().string() + "_" + fs::path(fileB).stem().string() + "_diff_report.md";
        ofstream out(outName);
        out << report;
        out.close();

        cout << "[SUCCESS] Report generated: " << outName << endl;
        return 0;
    }

    // --- STANDARD COMPILATION LOGIC ---
    vector<string> inputFiles; 
    string outputName = "";
    string mode = "local"; 
    bool explicitLang = false;
    bool dryRun = false;
    bool updateMode = false; 
    bool runOutput = false;
    bool keepSource = false;
    bool transpileMode = false;

    for(int i=1; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i+1 < argc) { outputName = argv[i+1]; i++; }
        else if (arg == "-cloud") mode = "cloud";
        else if (arg == "-local") mode = "local";
        else if (arg == "-dry-run") dryRun = true;
        else if (arg == "-verbose") VERBOSE_MODE = true;
        else if (arg == "-u" || arg == "--update") updateMode = true;
        else if (arg == "-run" || arg == "--run") runOutput = true;
        else if (arg == "-k" || arg == "--keep") keepSource = true;
        else if (arg == "-t" || arg == "--transpile") transpileMode = true;
        else if (arg == "--version") { cout << "Yori Compiler v" << CURRENT_VERSION << endl; return 0; }
        else if (arg == "--clean") {
            cout << "[CLEAN] Removing temporary build files..." << endl;
            try {
                if (fs::exists(".yori_build.cache")) fs::remove(".yori_build.cache");
                for (const auto& entry : fs::directory_iterator(fs::current_path())) {
                    if (entry.is_regular_file()) {
                        string fname = entry.path().filename().string();
                        if (fname.find("temp_build") == 0) fs::remove(entry.path());
                    }
                }
            } catch (...) {}
            return 0;
        }
        else if (arg == "--init") {
            cout << "[INIT] Creating project template..." << endl;
            if (!fs::exists("hello.yori")) {
                ofstream f("hello.yori");
                f << "// Welcome to Yori!\nPRINT(\"Hello, World!\")\n";
                f.close();
            }
            if (!fs::exists("config.json")) {
                ofstream f("config.json");
                f << "{\n    \"local\": {\n        \"model_id\": \"qwen2.5-coder:3b\",\n        \"api_url\": \"http://localhost:11434/api/generate\"\n    },\n    \"cloud\": {\n        \"protocol\": \"openai\",\n        \"api_key\": \"YOUR_KEY\",\n        \"model_id\": \"llama3-70b-8192\",\n        \"api_url\": \"https://api.groq.com/openai/v1/chat/completions\"\n    }\n}\n";
                f.close();
            }
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            // Already handled at start, just redundancy check
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

    size_t currentHash = hash<string>{}(aggregatedContext + CURRENT_LANG.id + MODEL_ID + (updateMode ? "u" : "n"));
    string cacheFile = ".yori_build.cache"; 

    if (!updateMode && !dryRun && fs::exists(cacheFile) && fs::exists(outputName)) {
        ifstream cFile(cacheFile);
        size_t storedHash;
        if (cFile >> storedHash && storedHash == currentHash) {
            cout << "[CACHE] No changes detected. Using existing build." << endl;
            if (runOutput) {
                // ... same run logic as below ...
                #ifdef _WIN32
                system(outputName.c_str());
                #else
                string cmd = "./" + outputName; system(cmd.c_str());
                #endif
            }
            return 0;
        }
    }

    set<string> potentialDeps = extractDependencies(aggregatedContext);
    if (!preFlightCheck(potentialDeps)) return 1;

    string existingCode = "";
    if (updateMode) {
        string srcPath = stripExt(outputName) + CURRENT_LANG.extension;
        if (fs::exists(srcPath)) {
            ifstream old(srcPath);
            existingCode.assign((istreambuf_iterator<char>(old)), istreambuf_iterator<char>());
            cout << "   [UPDATE] Found existing source: " << srcPath << endl;
        }
    }

    if (dryRun) { cout << "--- CONTEXT PREVIEW ---\n" << aggregatedContext << endl; return 0; }

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
        if (!errorHistory.empty()) prompt << "\n[!] PREVIOUS ERRORS:\n" << errorHistory << "\n";
        prompt << "\nOUTPUT: Only code.";

        string response = callAI(prompt.str());
        string code = extractCode(response);
        
        if (code.find("ERROR:") == 0) { 
            cout << "   [!] API Error: " << code.substr(6) << endl; 
            log("API_FAIL", code); 
            if (code.find("JSON Parsing Failed") != string::npos) {
                 cout << "       (Hint: Check 'yori config cloud-protocol'. Current: " << PROTOCOL << ", Provider URL: " << API_URL << ")" << endl;
            }
            continue; 
        }

        ofstream out(tempSrc); out << code; out.close();

        cout << "   Verifying..." << endl;
        string valCmd = CURRENT_LANG.buildCmd + " \"" + tempSrc + "\"";
        if (CURRENT_LANG.producesBinary) valCmd += " -o \"" + tempBin + "\""; 
        
        CmdResult build = execCmd(valCmd);
        
        if (build.exitCode == 0) {
            cout << "\nBUILD SUCCESSFUL: " << outputName << endl;
            std::error_code ec;
            bool saveSuccess = false;

            // Smart Output Logic
            bool saveAsSource = (getExt(outputName) == CURRENT_LANG.extension) || transpileMode;

            for(int i=0; i<5; i++) {
                try {
                    if (fs::exists(outputName)) fs::remove(outputName);
                    
                    if (CURRENT_LANG.producesBinary && !saveAsSource) {
                        fs::copy_file(tempBin, outputName, fs::copy_options::overwrite_existing);
                        cout << "   [Binary]: " << outputName << endl;
                        
                        if (keepSource) {
                            string sName = stripExt(outputName) + CURRENT_LANG.extension;
                            fs::copy_file(tempSrc, sName, fs::copy_options::overwrite_existing);
                            cout << "   [Source Kept]: " << sName << endl;
                        }
                        fs::remove(tempBin);
                    } else {
                        fs::copy_file(tempSrc, outputName, fs::copy_options::overwrite_existing);
                        cout << "   [Source]: " << outputName << endl;
                        if (CURRENT_LANG.producesBinary && fs::exists(tempBin)) fs::remove(tempBin);
                    }
                    saveSuccess = true;
                    break;
                } catch (const fs::filesystem_error& e) {
                    if (i < 4) std::this_thread::sleep_for(std::chrono::milliseconds(250 * (i + 1)));
                    else ec = e.code();
                }
            }

            if (!saveSuccess) {
                cerr << "[ERROR] Failed to save final output. File may be locked." << endl;
                cout << "   Your build is preserved at: " << (CURRENT_LANG.producesBinary ? tempBin : tempSrc) << endl;
                return 1;
            }
            
            if (fs::exists(tempSrc) && !keepSource) fs::remove(tempSrc, ec);
            ofstream cFile(cacheFile); cFile << currentHash;

            if (runOutput) {
                cout << "\n[RUN] Executing..." << endl;
                string cmd = outputName;
                if (!CURRENT_LANG.producesBinary) {
                    string interpreter = CURRENT_LANG.buildCmd.substr(0, CURRENT_LANG.buildCmd.find(' '));
                    cmd = interpreter + " \"" + outputName + "\"";
                } else {
                    #ifndef _WIN32
                    if (outputName.find('/') == string::npos) cmd = "./" + outputName;
                    fs::permissions(outputName, fs::perms::owner_exec, fs::perm_options::add, ec);
                    #endif
                    cmd = "\"" + cmd + "\"";
                }
                system(cmd.c_str());
            }

            return 0;
        } else {
            string err = build.output;
            cout << "   [!] Error (Line " << gen << "): " << err.substr(0, 50) << "..." << endl;
            log("FAIL", "Pass " + to_string(gen) + " failed.");

            if (isFatalError(err)) {
                cerr << "\n[FATAL ERROR] Missing dependency/file detected. Aborting." << endl;
                cout << "   [?] Analyze fatal error with AI? [y/N]: ";
                char ans; cin >> ans;
                if (ans == 'y' || ans == 'Y') explainFatalError(err);
                break; 
            }
            errorHistory += "\n--- Error Pass " + to_string(gen) + " ---\n" + err;
        }
    }

    cerr << "Failed to build after " << passes << " attempts." << endl;
    return 1;
}