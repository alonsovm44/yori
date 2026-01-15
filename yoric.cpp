/* YORI COMPILER (yori.exe) - Configurable Version
   Uso: yori archivo.yori -o salida.exe
   Requiere: config.json en el directorio actual.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <cstdio>  
#include <cstdlib> 

#include "json.hpp" 

using json = nlohmann::json;
using namespace std;

// --- VARIABLES GLOBALES (Ya no son const hardcodeadas) ---
string API_KEY = "";
string MODEL_ID = "gemini-2.5-flash"; // Default por si acaso
const int MAX_RETRIES = 5;

// --- PARCHE DE COMPATIBILIDAD WINDOWS/LINUX ---
#ifdef _WIN32
extern "C" {
    FILE* _popen(const char* command, const char* mode);
    int _pclose(FILE* stream);
}
#else
    #define _popen popen
    #define _pclose pclose
#endif
// ----------------------------------------------

// --- GESTIÓN DE CONFIGURACIÓN ---
bool loadConfig() {
    ifstream f("config.json");
    if (!f.is_open()) {
        return false;
    }
    try {
        json j = json::parse(f);
        
        // Cargar API KEY
        if (j.contains("api_key")) {
            API_KEY = j["api_key"];
        } else {
            cerr << "❌ Error en config.json: Falta el campo 'api_key'." << endl;
            return false;
        }

        // Cargar Modelo (Opcional, tiene default)
        if (j.contains("model_id")) {
            MODEL_ID = j["model_id"];
        }
        
        return true;
    } catch (exception& e) {
        cerr << "❌ Error parseando config.json: " << e.what() << endl;
        return false;
    }
}

// --- SISTEMA (Ejecutar comandos) ---
string execCmd(string cmd) {
    array<char, 128> buffer;
    string result;
    string full_cmd = cmd + " 2>&1"; 

    FILE* pipe = _popen(full_cmd.c_str(), "r");
    if (!pipe) return "EXEC_FAIL";

    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    } catch (...) {
        _pclose(pipe);
        throw;
    }

    _pclose(pipe);
    return result;
}

// --- RED (Llamada a la IA) ---
string callAI(string prompt) {
    string response;
    int waitTime = 30; // Segundos de espera si falla

    while (true) {
        json body;
        body["contents"][0]["parts"][0]["text"] = prompt;
        
        ofstream file("request_temp.json");
        file << body.dump();
        file.close();

        string url = "https://generativelanguage.googleapis.com/v1beta/models/" + MODEL_ID + ":generateContent?key=" + API_KEY;
        string cmd = "curl -s -X POST -H \"Content-Type: application/json\" -d @request_temp.json \"" + url + "\"";

        response = execCmd(cmd);
        remove("request_temp.json");

        // Chequeo de Errores de Cuota
        if (response.find("429") != string::npos || response.find("Quota exceeded") != string::npos) {
            cout << "\n⏳ [RATE LIMIT] Google dice que esperemos. Durmiendo " << waitTime << "s..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(waitTime));
            // No hacemos break, el while loop lo intentará de nuevo
            continue; 
        }

        // Si no es error de cuota, salimos del bucle y devolvemos lo que sea que haya llegado
        break;
    }

    return response;
}

string cleanCode(string raw) {
    size_t start = raw.find("```");
    if (start == string::npos) return raw;
    size_t end_line = raw.find('\n', start);
    size_t end_block = raw.rfind("```");
    if (end_line != string::npos && end_block != string::npos && end_block > end_line) {
        return raw.substr(end_line + 1, end_block - end_line - 1);
    }
    return raw;
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    // 0. CARGAR CONFIGURACIÓN
    if (!loadConfig()) {
        cerr << " FATAL: No se pudo cargar 'config.json'." << endl;
        cerr << "   Asegurate de crear el archivo con tu 'api_key' en la misma carpeta." << endl;
        return 1;
    }

    // 1. Parsing Argumentos
    if (argc < 2) {
        cout << "Uso: yori <archivo.yori> [-o <salida.exe>]" << endl;
        return 0;
    }

    string inputFile = argv[1];
    string outputFile = "a.exe";
    
    for(int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i+1 < argc) outputFile = argv[i+1];
    }

    cout << " [YORI COMPILER] Motor: " << MODEL_ID << endl;
    cout << "   Procesando " << inputFile << "..." << endl;

    ifstream f(inputFile);
    if (!f.is_open()) {
        cerr << " Error: No se encuentra " << inputFile << endl;
        return 1;
    }
    string yoriCode((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());

    string tempCpp = "temp_build.cpp";
    string currentError = "";

    // 2. Bucle Evolutivo
    for(int gen=1; gen<=MAX_RETRIES; gen++) {
        cout << "   [Pass " << gen << "] Generando C++..." << endl;
        
        string prompt;
        if (currentError.empty()) {
            prompt = "ROLE: Expert C++ Compiler.\nTASK: Translate this Yori code to a SINGLE, COMPLETE C++ file.\nSOURCE YORI:\n" + yoriCode + "\nOUTPUT: Only C++ code.";
        } else {
            cout << "   [EVOLUCION] Corrigiendo error..." << endl;
            prompt = "ROLE: Expert C++ Debugger.\nTASK: Fix this C++ code based on the error.\nERROR:\n" + currentError + "\nORIGINAL YORI:\n" + yoriCode + "\nOUTPUT: Only corrected C++ code.";
        }

        string response = callAI(prompt);
        
        // Validación básica
        if (response.find("error") != string::npos && response.find("code") != string::npos) {
             try {
                 json j = json::parse(response);
                 if (j.contains("error")) {
                     cerr << "API Error: " << j["error"]["message"] << endl;
                     if (j["error"]["message"] == "API_KEY_INVALID") {
                         cerr << "   >>> Verifica tu clave en config.json" << endl;
                     }
                     return 1;
                 }
             } catch (...) {}
        }

        try {
            json j = json::parse(response);
            if (!j.contains("candidates")) { 
                currentError = "Network/API Error: " + response;
                continue; 
            }
            string raw = j["candidates"][0]["content"]["parts"][0]["text"];
            string code = cleanCode(raw);
            
            ofstream out(tempCpp);
            out << code;
            out.close();
        } catch (exception& e) {
            cerr << " JSON Parse Error: " << e.what() << endl;
            continue;
        }

        cout << "   Compiling..." << endl;
        string cmd = "g++ " + tempCpp + " -o " + outputFile; 
        
        string output = execCmd(cmd);

        if (output.find("error:") == string::npos && output.find("fatal error:") == string::npos) {
            cout << "\n BUILD SUCCESSFUL: " << outputFile << endl;
            remove(tempCpp.c_str());
            return 0;
        } else {
            if (output.find("No such file") != string::npos && output.find(".h") != string::npos) {
                cerr << "\n FATAL: Falta una libreria en tu sistema." << endl;
                return 1;
            }
            currentError = output;
        }
    }

    cerr << "\n BUILD FAILED." << endl;
    return 1;
}