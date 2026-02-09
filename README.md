# Yori Compiler
The AI-Native Compiler.The first build system that bridges the gap between Human Intent and Native Machine Code.
--- 
## What is Yori?
Traditional compilers (GCC, Clang) are rigid: they translate syntax. If you make a mistake, they complain.Yori is different. It is an "Agentic" compiler that understands Intent.

It doesn't just translate; it comprehends. If Yori encounters an error (syntax, missing library, logic flaw), it stops, analyzes the failure, fixes the source code, and recompiles automatically. It turns your natural language and pseudo-code into high-performance binaries (C++, Rust, Go) instantly.

## Why use Yori?
1. The Evolution of Build Systems (Beyond CMake)
CMake revolutionized software by bridging the gap between Configuration and Build.Yori takes this to the next extreme. It bridges the gap between Concept and Implementation.

A. With CMake: You write configuration files. You still have to write the C++ code yourself.
B. With Yori: You write the Intent (e.g., "Create a dashboard"), and Yori generates the configuration, the source code, and the build files.

2. Speed of Prototyping (Zero-Syntax)
Transform a high-level idea into a compiled executable in minutes, not days.

Input: English intent + Pseudo-code.
Output: Optimized C++ or Rust binaries.
Value: Reduces "Time-to-Binary" from weeks to minutes.

3. Self-Healing Toolchain
Stop debugging compiler errors manually. Yori creates a feedback loop:

Generate Code. Compile. Fail? Yori reads the error, patches the code, and retries.
## Key Features
1. Semantic Transpilation (Code Agnostic)
Write in Python, JavaScript, or your own Domain Specific Language (DSL) and compile it to native performance. Yori guarantees native implementation (no "Python.h" hacks), creating standalone executables.

2. Universal Imports (IMPORT:)
Mix languages in a single project.
```acorn
IMPORT: "utils.py"  // Python logic
IMPORT: "core.cpp"  // C++ performance

INTENT "Combine core.cpp and utils.py in a single executable file"
```
## SECI (Semantic Embedded Code Injection)
Inject AI-generated logic directly into specific files using the $${ ... }$$ blocks.

```acorn
EXPORT: "main.cpp" 
#include <iostream>
 $${
   1. print hello world and clean the buffer
}$$
```
Yori will provide the cpp code in the semantic block, whatever is found outside it will be compied literally into the file.

### Model Agnostic
*   **Local:** Full support for Ollama (Llama 3, Qwen, DeepSeek). Zero cost, 100% privacy.
*   **Cloud:** Native integration with Groq, OpenAI, and Google Gemini. Faster, more advanced models.

---

## Use Cases

### Who is Yori for?
*   **Product Managers:** specifying features that auto-compile to working demos.
*   **Researchers/Scientists:** Simulating systems without learning complex syntax (C++/Rust).
*   **Students:** Learning computational thinking without wrestling with syntax errors.
*   **Indie Devs:** Rapidly scaffolding multilingual prototypes.

### The "Zero-Syntax" Pipeline
The Problem: High-performance languages (C++, Rust) have steep learning curves.
The Solution: Write logic in Python/English. Use Yori to transpile to native C++ automatically.

---

## Installation

### Quick Install (Recommended)

**Linux/macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/alonsovm44/yori/master/linux_installer.sh | bash
``` 

**Windows:**
Windows (PowerShell):

```PS
irm https://raw.githubusercontent.com/alonsovm44/yori/master/install.ps1 | iex
```
Manual Build

````bash
g++ yoric.cpp -o yori -std=c++17 -lstdc++fs
```` 
Usage Examples
1. Basic Compilation
````bash
yori main.cpp -o app.exe -cpp -cloud
````

2. The Architect Mode (Multi-file Projects)
Generate entire project structures with EXPORT:.

```bash
yori project.txt -make -cloud
```
3. Auto-Fix Code
Let Yori solve compilation errors for you.
```bash
yori fix server.cpp "fix segmentation fault" -cloud
```
4. Universal Linking
Transpile multiple source files into a single binary.
````bash

yori a.py b.cpp c.rs -o app.exe -cpp -cloud
````
## What Yori is NOT
To manage expectations, it is important to distinguish Yori from traditional tools:

1. Not a Traditional Deterministic Compiler: Yori does not use a hand-written lexer or formal grammar rules. It uses Large Language Models (LLMs) to interpret intent.
2. Not 100% Deterministic: Due to the probabilistic nature of AI, compiling the same file twice might result in slightly different underlying C++ implementations (though functionally identical).
Mitigation: Yori employs caching mechanisms and strict prompting to stabilize output.
3. Not just a Wrapper: Unlike standard ChatGPT web interfaces, Yori adds systems engineering: Context Awareness, Dependency Checking, and an Auto-Healing Loop.

## Configuration
Setup Local AI (Privacy First)
```bash

yori config model-local qwen2.5-coder:latest
```
Setup Cloud AI (Max Reasoning)
```bash
yori config api-key "YOUR_KEY"
yori config model-cloud gemini-1.5-flash
```
The Vision
Programming has traditionally required years of study to master syntax and memory management. Yori aims to lower the barrier of software engineering so a broader audience can access computational resources for their professional fields.

Yori transforms the compiler from a syntax checker into a partner in creation.
---
License & Contributing
MIT License
---