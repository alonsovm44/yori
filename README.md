# Yori Compiler

The **Yori Compiler** is a meta-compilation tool designed to bridge the gap between high-level human intent and low-level machine execution. Its primary mission is to make programming accessible to everyone by allowing software to be built using natural language blueprints.

---

## The Vision
Programming has traditionally required years of study to master syntax and memory management. **Yori** changes this paradigm by turning the developer into an **Architect**. You provide the "What," and the Yori Engine—powered by local and cloud-based AI—determines the "How," generating and self-correcting C++ code until it produces a functional binary.


---

## Key Features

* **Natural Language to Binary**: Compile `.yori` files containing descriptive logic directly into executable `.exe` files.
* **Hybrid AI Core**:
    * **Cloud Mode**: Uses Google Gemini (e.g., `gemini-2.5-flash`) for high-speed, high-quality code generation. (Requires API Key)
    * **Local Mode**: Uses Ollama (e.g., `qwen2.5-coder`) for private, offline, and quota-free development.
* **Incremental Refinement (`-u`)**: Update existing C++ applications without rewriting them from scratch. Yori reads your previous source code and applies new changes intelligently.
* **Modular Architecture (`IMPORT:`)**: Build complex software by splitting logic into multiple `.yori` files. The compiler automatically links them into a single "Unity Build."
* **Genetic Evolution Engine**: If the generated code fails to compile, Yori captures the error and "evolves" the source code in multiple passes until it works.
* **Zero-Dependency Design**: The compiler uses system-level tools like `curl` and `g++`, making the final binaries lightweight and portable.

---

## Technical Setup

### Prerequisites
1.  **C++ Compiler**: [MinGW-w64](https://www.mingw-w64.org/) (Windows) or `g++` (Linux) added to system PATH.
2.  **Ollama (Optional)**: Required for Local Mode.
    * Run `ollama pull qwen2.5-coder:3b` to download the recommended model.
3.  **JSON Library**: Ensure `json.hpp` is present in the project folder.

### Configuration (`config.json`)
You can configure both local and cloud profiles simultaneously. Yori will switch based on the flags you use.

 # Usage 
Command Syntax
PowerShell

yori <main_file.yori> [-local | -cloud] [-o output.exe] [-k] [-u]
Flags

-local: Uses the local Ollama engine (Default).

-cloud: Uses the Google Gemini API (Requires API Key).

-o <name>: Specifies the output filename (e.g., app.exe).

-k: Keeps the generated C++ source file (.cpp) for inspection.

-u: Update Mode. Reads the existing .cpp file and modifies it based on new instructions in the .yori file.

# Advanced Features
1. Incremental Updates (-u)
Don't rebuild from scratch! If you have a working app.exe (and app.cpp), you can add a feature easily:

Edit app.yori: Add // NEW: Add a reset button.

Run: yori app.yori -u

Yori keeps your old logic and only inserts the new button.

2. Modularity (IMPORT)
For larger projects, split your code. 
math.yori vvvvv

MODULE: MathLib
// Define a function that calculates the factorial of a number.

_______________________________________________________________
main.yori vvvvv

PROGRAM: Calculator
IMPORT: math.yori
// Ask user for a number and use the factorial function from MathLib.
Yori will automatically merge these into a single valid program.

# Quick Start Tutorial

## Step 1: Create a Blueprint (hello.yori)


PROGRAM: HelloWorld
language: C++

// Create a program that asks for the user's name.
// Then, print a greeting message saying "Welcome to Yori, [Name]!".
// Finally, print a loop counting down from 3 to 1.
## tep 2: Compile
PowerShell

.\yori.exe hello.yori -local -o hello.exe -k

You will see the engine "Thinking" (Generating C++) and then "Compiling".

Step 3: Run Your App
PowerShell

.\hello.exe

It is planned to make future versions of yori so it builds in any language. Have fun coding!