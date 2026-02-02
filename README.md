# Yori Compiler

The **Yori Compiler** is a meta-compilation tool designed to bridge the gap between high-level human intent and machine execution. Its mission is to make programming accessible to everyone by allowing software to be built using natural language blueprints. It can also produce 3D object files from pseudocode or plain text description as well as vector images, such as graphs.

---

## Manual install (Windows)
1. Go to Releases and download the latest release, unzip and run the [installer.ps1]
2. To run the script right-click → Run with PowerShell
3. Follow prompts (Ollama, model, compiler auto-setup)


##  The Vision
Programming has traditionally required years of study to master syntax and memory management. Yori aims to lower the entry barrier of software engineering so a broader audience can access computational resources for their professional fields.  

# Yori is best meant for

- **Software developers** who want to prototype an MVP fast and better, from weeks to days or hours.
- **CS professors and CS101 students** who want to exercise computational thinking without wrestling syntax again.
- **Domain professionals** (physicists, biologists, economists) who need to simulate systems but don't understand language syntax—Yori lets them code in their domain vocabulary and transpile their code to working python scripts or native c++ executables.
- **Indie gamedevs** building multilingual prototypes and rapid iteration builds.
- **Data scientists** needing Python ML models packaged as single standalone executables.
- **Junior developers** who want to understand systems without syntax pain.
- **Product managers** specifying features that auto-compile to working demos for stakeholder validation.
- **DevOps engineers** generating infrastructure-as-code and deployment configs from high-level intent.
- **UX/Design teams** creating functional prototypes to validate interaction flows before frontend dev.
- **Hardware engineers** who need embedded firmware prototypes without toolchain setup.
- **Financial analysts** building risk models, dashboards, and trading algorithms without learning Python/R.
- **Marketers** generating landing pages, A/B tests, and conversion funnels as standalone deploys.
- **Open source maintainers** rapidly scaffolding new features, APIs, or CLIs for contributor feedback.
- **Technical writers** embedding working code examples directly in documentation that auto-updates.
- **CEO/Founders** sketching SaaS ideas that become shippable MVPs for investor demos.

---

## Quick Install
Quick Install Commands
Linux/macOS
<curl -fsSL https://raw.githubusercontent.com/alonsovm44/yori/master/install.sh | bash>
Or with wget:
<wget -qO- https://raw.githubusercontent.com/alonsovm44/yori/master/install.sh | bash>
Windows (PowerShell)
<irm https://raw.githubusercontent.com/alonsovm44/yori/master/install.ps1 | iex>
Or with shorter URL (if you setup a domain):
<irm yori.dev/install.ps1 | iex>
## Installation

### Quick Install (Recommended)

**Linux/macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/alonsovm44/yori/master/install.sh | bash
```

**Windows:**
```powershell
irm https://raw.githubusercontent.com/alonsovm44/yori/master/install.ps1 | iex
```

### Manual Install

1. Download the [latest release](https://github.com/alonsovm44/yori/releases)
2. Extract the archive
3. Run the installer:
   - Windows: `install.bat`
   - Linux: `./install.sh`

### Build from Source
```bash
git clone https://github.com/alonsovm44/yori
cd yori
g++ -std=c++17 yoric.cpp -o bin/yori
./bin/yori --version
```

---

## System Requirements

- **OS**: Windows 10+, Linux (Ubuntu 20.04+, Fedora 33+, Arch)
- **Compiler**: g++ with C++17 support
- **Optional**: Ollama (for local AI mode)

---

## First Steps

After installation:
```bash
# Create example project
yori --init

# Compile your first program
yori hello.yori -o hello.exe -c

# Run it
./hello.exe
```


---
##  What Yori IS NOT

To manage expectations regarding traditional compiler theory and define the tool's scope, it is important to clarify what Yori is not:

-Not a Traditional Deterministic Compiler:

-Unlike GCC, Clang, or Rustc, Yori does not rely on a hand-written lexer, parser, Abstract Syntax Tree (AST), or formal grammar rules to translate code.

-What it is instead: It is an Agentic Build System that leverages Large Language Models (LLMs) to interpret intent and transpile mixed-syntax code into a high-performance target language (like C++).

-Not 100% Deterministic (but can do incremental development with the -u flag):

Due to the probabilistic nature of AI, compiling the same source file twice might result in slightly different underlying C++ implementations (though functionally identical).

Mitigation: Yori employs caching mechanisms and strict prompting to stabilize output, but it does not guarantee bit-for-bit binary reproducibility across builds.

Not "Just a ChatGPT Wrapper":

While it uses AI for code generation, Yori adds a critical layer of Systems Engineering:

1. Context Awareness: Handles multi-file imports and dependencies.

2. Self-Healing: Automatically feeds compiler errors back into the system to fix bugs.

3. Fail-Fast Safety: Detects missing libraries/headers before wasting resources.

4. Toolchain Abstraction: Manages the invocation of native compilers (g++, rustc, etc.) transparently.

### Summary:
 Yori is a Software 2.0 tool. If you need absolute formal verification and clock-cycle precision, use a traditional compiler. If you want to prototype complex ideas in seconds using natural language and mixed logic or your own language, use Yori.

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
````
````b.py
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
Yori follows a You-provide-first philosophy, it won't make libraries from scratch, be sure to provide them if possible. Yori can handle this
````
INCLUDE: raylib.h
INCLUDE: mypy.py

//program logic

````
Yori behaves better with All-in-one libraries
## UNIVERSAL LINKING
Yori now supports universal file compilation (4.5) like so
````
>yori a.cpp b.py c.cs d.js e.acn -o myapp.exe -c -cloud 
//you could also transpile multiple files into a single file
>yori a.py b.py c.py -o d.c -cloud
````
## IMPROVE PERFORMANCE JUST WITH COMMENTS
Compiler directives are universal and can be used in any language.
if you have a python script (or any script) that needs a performance boost just add "#!!! improve performance" (or use the appropiate comment syntax with !!! flag) to tell the compiler what you want. Then you have two options
````
>yori myscript.py -o myscripy_boost.py -cloud
````
````Or
yori myscript.py -o myapp_boost.exe -cpp -cloud
````
 FAQ
 1. Do I need to install all 20 languages?
 A: No. If you try to build a Rust app but don't have rustc, Yori will alert you and ask if you want to proceed in "Blind Mode" (generate code without validation).
 2. Can I use this for interpreted languages?
 A: Yes. For languages like Python or JS, Yori uses the interpreter (like node -c) to check for syntax errors before saving the file.
 3. Is the generated code safe?
 A: Treat it like code from StackOverflow. Review it before running it in production...