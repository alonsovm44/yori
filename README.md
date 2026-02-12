# Yori 
Yori is a free and open-source CLI tool that helps you generate and build code using language models. It supports a simple multi-file DSL (EXPORT: blocks) and can retry compilation automatically by feeding compiler errors back into the model (self-healing).
Generate complete projects from natural language.
100% local. 100% private. 100% free.
No subscriptions. No telemetry. No cloud.
Your code, your machine, your rules.
Big Tech will eventually offer semantic blocks as a service.
We offer it as software.
--- 
## What is Yori?
Traditional compilers (GCC, Clang, rustc) translate code based on syntax and do not attempt to fix errors for you. When a build fails, you are left to interpret compiler messages, search documentation, and debug the issue manually.

Yori is a command-line tool that sits between your intent (written in plain text or pseudo-code) and your existing build tools. It uses a configured language model (local or cloud) to generate source code, writes the output to disk, and optionally runs the compiler or build script.

If compilation fails, Yori can attempt to fix the problem by re-running the model with the compiler error output, up to a configurable number of retries.

Yori is not a deterministic compiler or a formal transpiler. It is an orchestrator that relies on external compilers and the quality of the configured language model, using LLMs to assist with code generation and build orchestration

---
## Key Features
Key Features

1. LLM-based code generation from text input or existing files, which can be combination of any language, example:
`yori utils.py myalgorithm.c -o myprogram.exe -cpp -cloud`

2. Multi-file output via EXPORT: blocks in yori text files. Any text file will work for yori files, i recommend `.yori`, `.txt` or markdown files.

Note: yori files support user-generated code mixed with AI generated code using the $${ ... }$$ blocks. Example:
```yori
EXPORT: "mylib.h"
$${
  define a function called 'myfunction()' which takes a number and returns its square
}$$
EXPORT: END

EXPORT: "myprogram.cpp"
#include <iostream>
#include <vector>
#include "mylib.h" //for myfunction()

using namespace std;

int main(){
  int x = 3;
  $${
    make a vector V containing [1,2,3,4,5]
    print hello world and the vector V content and clean the buffer
    print(myfunction(x))  //prints 9
  }$$
}
EXPORT: END
``` 
You then run
```bash
yori idea.txt -make -cloud -series
```
As you can see, it is a high level text file with a C++ file embedded, which in turn has high level logic embedded into it. This allows the user to have full control on where the A.I touches the code vs where it is not allowed. The `-series` flag tells Yori to build the files in series, not in parallel (by default), this is to ensure the AI does not suffer from prompt fatigue and returns an incomplete or hallucinated output.

---

### TL;DR:
Yori parses the export blocks and creates the requested project files, copying the the textual code you provided and leting the AI fill the semantic patches you declared, using the `-series` flag tells Yori to make each file in series, not in parallel, to avoid prompt fatigue. Unlike most AI idea-to-code generators which work on an "all or nothing" basis, Yori gives you full control on where you want the AI to touch your code, and where you take the wheel.
---

3. Automatic build detection (Makefile, CMakeLists.txt, build.sh, build.bat)

4. Retry loop with compiler feedback (self-healing)

5. Optional execution of built binaries via `-run` flag

6. Model-agnostic (local and cloud models supported via ollama)

Additional utility commands: 
1. `fix`. Adds deltas to your file based in a text instruction
usage `yori fix project.c "fix segfault in line 1023" -local`
2. `explain`. It makes a copy of your file thoroughly commented anad explaining what the code does. Good for onboarding.
usage `yori explain main.cpp -cloud english`
3. `diff`, used to make a semantic diff markdown report
used like this `yori diff version1.py version2.py -cloud` 
4. `sos`. Custom techsupport in your terminal.
used like this `yori sos english -local "problem or error description"`

## Why use Yori?
1. A different approach to build automation

Traditional build systems (Make, CMake, etc.) focus on compiling and linking source code you already wrote, since they won't write code for you. Yori aims to help bridge the gap between intent and implementation by generating source code and build files based on a text description.

With CMake: you write configuration files and provide source code.

With Yori: you describe what you want in plain text, and Yori attempts to generate the source code and build scripts, then run the build.

2. Faster prototyping (with caveats)

Yori can accelerate early-stage prototyping by letting you express ideas in your own jargon or pseudo-code, then generating and compiling an initial implementation or MVP.

Input: Intent or pseudo-code
Output: Source code and optionally a compiled binary

Note: Results depend on the model, the explicitness and quality of the input. The output may also require refinement. Yori is not guaranteed to produce production-ready code. Treat the output Yori makes as a fresh piece out of a 3D printer.

3. Self-healing build loop

Yori can reduce time spent debugging compilation errors by using an automated feedbacks loop:

A. Generate code
B. Compile
C. If the build fails, send the compiler output back to the model
D. Retry (up to a configured number of attempts)
This can help with common compilation issues, but it is not a replacement for understanding the underlying code or dependencies.

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
## What Yori is NOT

To set clear expectations:

1. Not a compiler: It does not compile code itself. It relies on existing compilers and interpreters.
2. Not deterministic: Output depends on the model and prompt, so results may vary between runs.
3. Not a production build system: It does not track dependencies or perform incremental builds.
4. Not a formal transpiler: It generates code via LLMs, not via syntax tree translation.
```
The Vision
Programming has traditionally required years of study to master syntax and memory management. Yori aims to lower the barrier of software engineering so a broader audience can access computational resources for their professional fields.

Yori transforms the compiler from a syntax checker into a partner in creation.
---
License & Contributing
MIT License
---