/* YORI COMPILER (yori.exe) - v3.1 (Robust Incremental)
   Usage: yori file.yori [-cloud | -local] [-o output.exe] [-k] [-u]
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <cstdio>  
#include <cstdlib> 
#include <thread>
#include <chrono>

#include "json.hpp" 

using json = nlohmann::json;
using namespace std;

// --- DYNAMIC CONFIGURATION ---
string PROVIDER = "local"; 
string API_KEY = "";
string MODEL_ID = ""; 
string API_URL = "";
const int MAX_RETRIES = 5;

// --- COMPATIBILITY PATCH ---
#ifdef _WIN32
extern "C" {
    FILE* _popen(const char* command, const char* mode);
    int _pclose(FILE* stream);
}
#else
    #define _popen popen
    #define _pclose pclose
#endif

// --- CONFIGURATION LOADER (ROBUST v2.3) ---
bool loadConfig(string mode) {
    ifstream f("config.json");
    if (!f.is_open()) {
        cerr << "FATAL: config.json not found." << endl;
        return false;
    }
    
    try {
        json j = json::parse(f);
        if (!j.contains(mode)) { 
            cerr << "FATAL: Profile '" << mode << "' missing in config.json." << endl; 
            return false; 
        }

        json profile = j[mode];
        PROVIDER = mode;

        if (mode == "cloud") {
            if (!profile.contains("api_key") || profile["api_key"].get<string>().empty() || profile["api_key"].get<string>() == "YOUR_GEMINI_API_KEY_HERE") {
                cerr << "FATAL: Invalid or missing 'api_key' for cloud mode." << endl; 
                return false;
            }
            API_KEY = profile["api_key"];
        }

        if (!profile.contains("model_id")) {
             cerr << "FATAL: Missing 'model_id'." << endl; return false;
        }
        MODEL_ID = profile["model_id"];
        
        if (mode == "local") {
            API_URL = profile.contains("api_url") ? profile["api_url"].get<string>() : "http://localhost:11434/api/generate";
        }
        
        return true;
    } catch (exception& e) { 
        cerr << "FATAL: Config Error: " << e.what() << endl;
        return false; 
    }
}

// --- SYSTEM CORE ---
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

string callAI(string prompt) {
    string response;
    while (true) {
        json body;
        string url;
        if (PROVIDER == "local") {
            body["model"] = MODEL_ID; 
            body["prompt"] = prompt; 
            body["stream"] = false;
            url = API_URL;
        } else {
            body["contents"][0]["parts"][0]["text"] = prompt;
            url = "https://generativelanguage.googleapis.com/v1beta/models/" + MODEL_ID + ":generateContent?key=" + API_KEY;
        }

        ofstream file("request_temp.json");
        file << body.dump();
        file.close();

        string cmd = "curl -s -X POST -H \"Content-Type: application/json\" -d @request_temp.json \"" + url + "\"";
        response = execCmd(cmd);
        remove("request_temp.json");

        if (PROVIDER == "cloud" && (response.find("429") != string::npos || response.find("Quota exceeded") != string::npos)) {
            cout << "\n[RATE LIMIT] Waiting 30s..." << endl;
            this_thread::sleep_for(chrono::seconds(30));
            continue; 
        }
        break;
    }
    return response;
}

// --- EXTRACT CODE (ROBUST v2.3 - RESTORED) ---
string extractCode(string jsonResponse) {
    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        
        // 1. Detailed Error Reporting
        if (j.contains("error")) {
            if (j["error"].is_object() && j["error"].contains("message")) {
                return "ERROR: " + j["error"]["message"].get<string>();
            }
            return "ERROR: Unknown API Error";
        }

        // 2. Extraction
        if (PROVIDER == "local") { 
            if (j.contains("response")) raw = j["response"]; 
        } else { 
            if (j.contains("candidates") && !j["candidates"].empty()) 
                raw = j["candidates"][0]["content"]["parts"][0]["text"]; 
        }

        if (raw.empty()) return "ERROR: Empty response from AI.";

        // 3. Markdown Cleaning
        size_t start = raw.find("```");
        if (start == string::npos) return raw;
        size_t end_line = raw.find('\n', start);
        size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos) return raw.substr(end_line + 1, end_block - end_line - 1);
        return raw;
    } catch (exception& e) { return "JSON_PARSE_ERROR: " + string(e.what()); }
}

string stripExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? fname : fname.substr(0, lastindex); 
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: yori <file.yori> [-u] [-o output.exe] [-k] [-local|-cloud]" << endl;
        return 0;
    }

    string inputFile = argv[1];
    string outputFile = "a.exe";
    string mode = "local"; 
    bool keepSource = false;
    bool updateMode = false;

    for(int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-cloud") mode = "cloud";
        if (arg == "-local") mode = "local";
        if (arg == "-o" && i+1 < argc) outputFile = argv[i+1];
        if (arg == "-k") keepSource = true;
        if (arg == "-u") updateMode = true;
    }

    if (!loadConfig(mode)) return 1;

    string sourceFile = stripExt(outputFile) + ".cpp";
    cout << "[YORI] Mode: " << PROVIDER << " | Update: " << (updateMode ? "ON" : "OFF") << endl;

    ifstream f(inputFile);
    if (!f.is_open()) { cerr << "Error: Yori file not found." << endl; return 1; }
    string yoriCode((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    // INCREMENTAL LOGIC (v3.0)
    string existingCpp = "";
    if (updateMode) {
        ifstream oldSrc(sourceFile);
        if (oldSrc.is_open()) {
            cout << "   Reading existing source: " << sourceFile << endl;
            existingCpp = string((istreambuf_iterator<char>(oldSrc)), istreambuf_iterator<char>());
        } else {
            cout << "   [INFO] No existing source found. Starting fresh." << endl;
            updateMode = false; 
        }
    }

    string tempCpp = "temp_build.cpp";
    string currentError = "";

    for(int gen=1; gen<=MAX_RETRIES; gen++) {
        cout << "   [Pass " << gen << "] Generating..." << endl;
        
        string prompt;
        // PROMPT LOGIC
        if (currentError.empty()) {
            if (updateMode) {
                prompt = "ROLE: Expert C++ Maintainer.\nTASK: Update this C++ code based on new instructions. KEEP existing logic if valid.\n\n[EXISTING C++]:\n" + existingCpp + "\n\n[NEW INSTRUCTIONS]:\n" + yoriCode + "\n\nOUTPUT: Full updated C++ code only.";
            } else {
                prompt = "ROLE: Expert C++ Compiler.\nTASK: Translate Yori to C++.\nSOURCE:\n" + yoriCode + "\nOUTPUT: Only C++ code.";
            }
        } else {
            prompt = "ROLE: Expert Debugger.\nTASK: Fix compilation error.\nERROR:\n" + currentError + "\nINTENT:\n" + yoriCode + "\nOUTPUT: Only corrected C++ code.";
        }

        string response = callAI(prompt);
        string code = extractCode(response);

        if (code.find("ERROR:") == 0 || code.find("JSON_PARSE_ERROR:") == 0) { 
            cerr << "AI Error: " << code << endl; 
            return 1; 
        }
        
        ofstream out(tempCpp);
        out << code;
        out.close();

        cout << "   Compiling..." << endl;
        string output = execCmd("g++ " + tempCpp + " -o " + outputFile);

        if (output.find("error:") == string::npos && output.find("fatal error:") == string::npos) {
            cout << "\nBUILD SUCCESSFUL: " << outputFile << endl;
            
            // AUTOMATIC SOURCE PRESERVATION
            string cleanName = stripExt(outputFile) + ".cpp";
            remove(cleanName.c_str());
            rename(tempCpp.c_str(), cleanName.c_str());
            
            if (keepSource || updateMode) {
                cout << "   Source updated: " << cleanName << endl;
            }
            return 0;
        }
        
        // Critical Missing Lib Check
        if (output.find("No such file") != string::npos && output.find(".h") != string::npos) {
            cerr << "\nFATAL: Missing library." << endl;
            cerr << output.substr(0, 150) << "..." << endl;
            return 1;
        }

        currentError = output;
    }
    cerr << "\nBUILD FAILED." << endl;
    return 1;
}