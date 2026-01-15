# Yori Compiler

The **Yori Compiler** is a meta-compilation tool designed to bridge the gap between high-level human intent and low-level machine execution. Its primary mission is to make programming accessible to everyone by allowing software to be built using natural language blueprints.

---

##  The Vision
Programming has traditionally required years of study to master syntax and memory management. **Yori** changes this paradigm by turning the developer into an **Architect**. You provide the "What," and the Yori Engine—powered by local and cloud-based AI—determines the "How," generating and self-correcting C++ code until it produces a functional binary.



---

##  Key Features

* **Natural Language to Binary**: Compile `.yori` files containing descriptive logic directly into executable `.exe` files.
* **Hybrid AI Core**:
    * **Cloud Mode**: Uses Google Gemini (e.g., `gemini-1.5-flash`) for high-speed, high-quality code generation.
    * **Local Mode**: Uses Ollama (e.g., `qwen2.5-coder`) for private, offline, and quota-free development.
* **Genetic Evolution Engine**: If the generated code fails to compile, Yori captures the error and "evolves" the source code in multiple passes until it works.
* **Zero-Dependency Design**: The compiler uses system-level tools like `curl` and `g++`, making the final binaries lightweight and portable.
* **Source Inspection**: Option to keep the generated C++ source code for manual review or editing.

---

##  Technical Setup

### Prerequisites
1.  **C++ Compiler**: [MinGW-w64](https://www.mingw-w64.org/) (for Windows) or `g++` (Linux) installed and added to your system PATH.
2.  **Ollama (Optional)**: Required only if you want to use Local Mode.
    * Run `ollama pull qwen2.5-coder:3b` to download the recommended model.
3.  **JSON Library**: Ensure `json.hpp` is present in the project folder.

### Configuration (`config.json`)
Create a `config.json` file in the root directory. You can configure both profiles with cloud or local based computing simultaneously.
 
## Usage ##
Command Syntax
PowerShell

>yori <file.yori> [-local | -cloud] [-o output.exe] [-k]

Flags
-local: Uses the local Ollama engine (Default if no flag is set).
-cloud: Uses the Google Gemini API (Requires API Key in config).
-o <name>: Specifies the name of the output executable.
-k: Keeps the intermediate C++ source file (e.g., saves it as my_app.cpp).

Quick Start Tutorial
Follow this mini-tutorial to build your first application with Yori.

Step 1: Create a Blueprint
Create a text file named hello.yori and write your idea in plain English (or Spanish):

YAML

PROGRAM: HelloWorld
TECH: C++

// Create a program that asks for the user's name.
// Then, print a greeting message saying "Welcome to Yori, [Name]!".
// Finally, print a loop counting down from 3 to 1.

Step 2: Compile 
Open your terminal and tell Yori to build it. We will use the -k flag to see the code it writes.

Using Local AI (Ollama):

PowerShell

.\yori.exe hello.yori -local -o hello.exe -k
Using Cloud AI:

PowerShell

.\yori.exe hello.yori -cloud -o hello.exe -k
You will see the engine "Thinking" (Generating C++) and then "Compiling".

Step 3: Run Your App
If the build was successful, you now have a real program!

PowerShell

.\hello.exe
Step 4: Inspect the Code
Since you used the -k flag, check the folder. You will find a file named hello.cpp. Open it to see how the AI translated your words into valid C++ code.

