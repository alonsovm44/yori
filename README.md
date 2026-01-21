## Recent Breakthrough

Yori has reached a major milestone: it can now **semantically include and orchestrate code written in any programming language** inside a single compilation pipeline. In practice, this means a Yori program can include a Python file (even using libraries like NumPy), express intent in natural language, and be compiled into a **native C executable** that runs successfully — without containers, manual FFI, or handwritten glue code. This demonstrates that Yori is not bound to any single language or runtime, but operates as a **universal, intent-driven semantic compiler**, capable of deciding *how* and *where* code should execute based purely on human intent.

In a general sense, you can compile any text file as source and include any library in the source file from any language, provided you give the path after the INCLUDE flag

>yori input.extension -o output.extension 

# Yori Compiler

The **Yori Compiler** is a meta-compilation tool designed to bridge the gap between high-level human intent and machine execution. Its mission is to make programming accessible to everyone by allowing software to be built using natural language blueprints.

---

##  The Vision
Programming has traditionally required years of study to master syntax and memory management. **Yori** changes this paradigm by turning the developer into an **Architect**. You provide the "What," and the Yori Engine—powered by local and cloud-based AI—determines the "How," generating, verifying, and self-correcting code in **over 20 programming languages** until it produces a working result.

---

##  Key Features

* **Universal Polyglot Support**: Generate and validate code in 20+ languages including **C++, Python, Rust, Go, TypeScript, Zig, Java, C#, and Bash**.
* **Natural Language to Code**: Compile `.yori` files containing descriptive logic directly into source code or executables.
* **Self-Healing Toolchain**:
    * **Compiled Languages** (C++, Go, Rust): Yori captures compiler errors and "evolves" the source code until it compiles.
    * **Interpreted Languages** (Python, JS, Ruby): Yori validates syntax and static typing before saving the file.
* **Hybrid AI Core**:
    * **Cloud Mode**: Uses Google Gemini (e.g., `gemini-1.5-flash`) for high-speed generation.
    * **Local Mode**: Uses Ollama (e.g., `qwen2.5-coder`) for private, offline development.
* **Interactive Mode**: If the output format is ambiguous (e.g., just `-o app`), Yori launches an interactive menu to let you select the target language.
* **Modular Architecture (`IMPORT:`)**: Build complex software by splitting logic into multiple `.yori` files.
* **Zero-Dependency Design**: The compiler uses system-level tools (like `curl`, `g++`, `node`) already present on your machine.

---

## Technical Setup

### Prerequisites
1.  **Yori Core**: The `yori.exe` executable.
2.  **Ollama (Optional)**: Required for Local Mode (`ollama pull qwen2.5-coder:3b`).
3.  **Language Toolchains**: Yori uses the tools you already have.
    * For C++: `g++`
    * For Python: `python`
    * For Node: `node`
    * For Rust: `rustc`
    * *(If a tool is missing, Yori will warn you and offer to generate code in "Blind Mode".)*

### Configuration (`config.json`)

 Usage
 Command Syntax
 PowerShell> yori <file.yori> [-o output_name] [LANGUAGE_FLAG] [-u] [-local | -cloud]
Language Flags (The Polyglot System)
Yori auto-detects the language from the output extension (e.g., -o app.py), but you can force specific languages with flags:FlagLanguageTool Used-cppC++g++
-py     Python
-ts     TypeScripttsc-
-rs     Rustrustc
-go     Gogo
-js     JavaScript
-zig    Zigzig
-cs     C#.net
-sh     Bash
-ps1    PowerShell pwsh...
-java   Java
-rb    Ruby 
-php    PHP
-lua    Lua 
-r      R 
-jl    Juli 
-hs   Haskell.

Other Flags
-o <name>: Specifies output filename.
-u: Update Mode. Reads existing code and modifies it based on new instructions.
-local / -cloud: Switch AI provider.

## UNIVERSAL IMPORTS

Yori files support universal imports using IMPORT: keyword. Example:

````a.cpp
int function1(int x){
    return x + 6;
}
``````
````````b.py
def function2(x):
    return x + 7
````
````example.yori
IMPORT: a.cpp //for function1()
IMPORT: b.py // for function2()

INT x=1
PRINT(function1(x))
PRINT(function2(y))

```` 
````Result
>yori example.yori -o myapp.exe -cpp -cloud 
>7
>8
````
## UNIVERSAL LINKING
Yori now supports universal file compilation (4.5) like so
````
>yori a.cpp b.py c.cs d.js e.acn -o myapp.exe -c -cloud 
//you could also transpile multiple files into a single file
>yori a.py b.py c.py -o d.c -cloud
````
 FAQ
 1. Do I need to install all 20 languages?
 A: No. If you try to build a Rust app but don't have rustc, Yori will alert you and ask if you want to proceed in "Blind Mode" (generate code without validation).
 2. Can I use this for interpreted languages?
 A: Yes. For languages like Python or JS, Yori uses the interpreter (like node -c) to check for syntax errors before saving the file.
 3. Is the generated code safe?
 A: Treat it like code from StackOverflow. Review it before running it in production.
