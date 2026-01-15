/* YORI COMPILER (yori.exe) - v2.3 Robust
   Usage: yori file.yori [-cloud | -local] [-o output.exe] [-k]
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

// --- CONFIGURATION LOADER ---
bool loadConfig(string mode) {
    ifstream f("config.json");
    if (!f.is_open()) {
        cerr << "FATAL: config.json not found in current directory." << endl;
        return false;
    }
    
    try {
        json j = json::parse(f);
        
        if (!j.contains(mode)) {
            cerr << "FATAL: Profile '" << mode << "' not found in config.json." << endl;
            return false;
        }

        json profile = j[mode];
        PROVIDER = mode;

        // Validation for Cloud
        if (mode == "cloud") {
            if (!profile.contains("api_key") || profile["api_key"].get<string>().empty() || profile["api_key"].get<string>() == "YOUR_GEMINI_API_KEY_HERE") {
                cerr << "FATAL: Missing or invalid 'api_key' for cloud mode." << endl;
                return false;
            }
            API_KEY = profile["api_key"];
        }

        // Validation for Model ID
        if (!profile.contains("model_id") || profile["model_id"].get<string>().empty()) {
            cerr << "FATAL: No 'model_id' detected for " << mode << " mode." << endl;
            return false;
        }
        MODEL_ID = profile["model_id"];

        // Validation for Local URL
        if (mode == "local") {
            API_URL = profile.contains("api_url") ? profile["api_url"].get<string>() : "http://localhost:11434/api/generate";
        }
        
        return true;
    } catch (json::parse_error& e) {
        cerr << "FATAL: config.json syntax error: " << e.what() << endl;
        return false;
    } catch (exception& e) {
        cerr << "FATAL: Config load error: " << e.what() << endl;
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

        // Rate Limit Check (Cloud Only)
        if (PROVIDER == "cloud" && (response.find("429") != string::npos || response.find("Quota exceeded") != string::npos)) {
            cout << "\n[RATE LIMIT] Waiting 30s..." << endl;
            this_thread::sleep_for(chrono::seconds(30));
            continue; 
        }
        break;
    }
    return response;
}

// --- ROBUST PARSER (Recuperado) ---
string extractCode(string jsonResponse) {
    try {
        json j = json::parse(jsonResponse);
        string raw = "";
        
        // 1. Check specific API errors first
        if (j.contains("error")) {
            if (j["error"].is_object() && j["error"].contains("message")) {
                return "ERROR: " + j["error"]["message"].get<string>();
            } else {
                return "ERROR: Unknown API error occurred.";
            }
        }

        // 2. Extract content based on provider
        if (PROVIDER == "local") {
            if (j.contains("response")) raw = j["response"].get<string>();
        } else {
            if (j.contains("candidates") && !j["candidates"].empty()) {
                raw = j["candidates"][0]["content"]["parts"][0]["text"].get<string>();
            }
        }

        if (raw.empty()) return "ERROR: Empty response from AI model.";

        // 3. Clean Markdown
        size_t start = raw.find("```");
        if (start == string::npos) return raw;
        size_t end_line = raw.find('\n', start);
        size_t end_block = raw.rfind("```");
        if (end_line != string::npos && end_block != string::npos && end_block > end_line) {
            return raw.substr(end_line + 1, end_block - end_line - 1);
        }
        return raw;

    } catch (exception& e) {
        // Return the raw response for debugging if parsing fails
        return "JSON_PARSE_ERROR: " + string(e.what()); 
    }
}

string stripExt(string fname) {
    size_t lastindex = fname.find_last_of("."); 
    return (lastindex == string::npos) ? fname : fname.substr(0, lastindex); 
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: yori <file.yori> [-local | -cloud] [-o output.exe] [-k]" << endl;
        return 0;
    }

    string inputFile = argv[1];
    string outputFile = "a.exe";
    string mode = "local"; // Default
    bool keepSource = false;

    for(int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-cloud") mode = "cloud";
        if (arg == "-local") mode = "local";
        if (arg == "-o" && i+1 < argc) outputFile = argv[i+1];
        if (arg == "-k") keepSource = true;
    }

    if (!loadConfig(mode)) return 1;

    cout << "[YORI COMPILER] Mode: " << PROVIDER << " | Model: " << MODEL_ID << endl;
    cout << "   Processing " << inputFile << "..." << endl;

    ifstream f(inputFile);
    if (!f.is_open()) { cerr << "Error: File not found." << endl; return 1; }
    string yoriCode((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    string tempCpp = "temp_build.cpp";
    string currentError = "";

    for(int gen=1; gen<=MAX_RETRIES; gen++) {
        cout << "   [Pass " << gen << "] Generating C++..." << endl;
        
        string prompt = currentError.empty() ? 
            "Translate to C++:\n" + yoriCode : 
            "Fix this C++ error:\n" + currentError + "\nOriginal intent:\n" + yoriCode;

        string response = callAI(prompt);
        string code = extractCode(response);

        // Explicit Error Trapping
        if (code.find("ERROR:") == 0 || code.find("JSON_PARSE_ERROR:") == 0) { 
            cerr << "AI Error: " << code << endl; 
            if (code.find("connect") != string::npos) {
                cerr << "   >>> Tip: Is Ollama running? (Run 'ollama serve')" << endl;
            }
            return 1; 
        }
        
        ofstream out(tempCpp);
        out << code;
        out.close();

        cout << "   Compiling..." << endl;
        string output = execCmd("g++ " + tempCpp + " -o " + outputFile);

        if (output.find("error:") == string::npos && output.find("fatal error:") == string::npos) {
            cout << "\nBUILD SUCCESSFUL: " << outputFile << endl;
            if (keepSource) {
                string cleanName = stripExt(outputFile) + ".cpp";
                remove(cleanName.c_str()); // Safety: remove old file before renaming
                if (rename(tempCpp.c_str(), cleanName.c_str()) == 0)
                    cout << "   Source saved: " << cleanName << endl;
                else
                    cerr << "   Warning: Could not rename source file." << endl;
            } else { 
                remove(tempCpp.c_str()); 
            }
            return 0;
        }
        
        // Check for missing libraries specifically
        if (output.find("No such file") != string::npos && output.find(".h") != string::npos) {
            cerr << "\nFATAL: Missing library in your system." << endl;
            cerr << output.substr(0, 200) << "..." << endl; // Show a bit of the error
            return 1;
        }

        currentError = output;
    }
    cerr << "\nBUILD FAILED." << endl;
    return 1;
}