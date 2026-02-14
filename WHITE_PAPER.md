# Intention is all you need

**Author:** Alonso Velazquez  
**Affiliation:** Sinaloa Autonomous University  
**Date:** February 2026  
**arXiv Category:** cs.SE (Software Engineering), cs.PL (Programming Languages)

---

## Abstract

We present **Yori**, a practical meta-compiler that enables developers to mix concrete code with natural language intent within the same source file. Using a simple delimiter syntax (`$${...}$$`), programmers can write hybrid programs where algorithmic details are specified in traditional code while boilerplate, utilities, or exploratory logic is expressed in natural language. Yori transpiles these semantic blocks to executable code across 30+ languages using Large Language Models (LLMs) as a backend, with multiple provider support (local Ollama, OpenAI, Google). 

Key features include: (1) inline semantic blocks that preserve surrounding code context, (2) multi-file project generation via `EXPORT` directives, (3) iterative compilation with error feedback to improve LLM output reliability, (4) pattern detection to prevent common LLM failure modes (wrapper attempts, lazy translations), and (5) integrated developer tools (code repair, documentation generation, semantic diff analysis). Our implementation demonstrates practical viability on real-world programs with a 68% first-pass success rate and 89% success within 5 retry attempts using local models.

**Keywords:** LLM-assisted programming, hybrid programming paradigm, semantic compilation, code generation, developer tools

---

## 1. Introduction

Large Language Models have shown remarkable ability to generate code from natural language descriptions. However, current LLM-assisted programming tools operate primarily through two paradigms:

1. **Chat interfaces** (ChatGPT, Claude): Full program generation through conversation, disconnected from existing codebases
2. **Autocomplete** (GitHub Copilot): Inline suggestions that complete code based on context

Both approaches treat natural language as **external** to the programming process. We propose a third paradigm: **treating natural language intent as first-class syntax** within source files.

### 1.1 Motivating Example

Consider a developer who knows the high-level algorithm but not the language-specific APIs:

```cpp
#include <vector>
#include <iostream>

int main() {
    $${
        create a vector with the first 10 prime numbers
        print each one with its index
        calculate and print the sum
    }$$
    return 0;
}
```

The `$${...}$$` blocks are not comments—they are **semantic placeholders** compiled to concrete code by an LLM while preserving the surrounding C++ structure (includes, main signature, return statement).

This enables:
- **Gradual specification**: Start with intent, refine to code incrementally
- **Language learning**: Express what you want, learn syntax from generated output  
- **Rapid prototyping**: Focus on architecture, delegate boilerplate
- **Mixed expertise**: Domain experts collaborate with language experts in same file

### 1.2 Contributions

This paper describes the design and implementation of Yori, a working compiler supporting this paradigm. Our contributions are:

1. **Lightweight syntax convention** for embedding semantic blocks in source code
2. **Multi-file orchestration** through `EXPORT` directives for project generation
3. **Iterative refinement with error feedback** to improve LLM reliability
4. **Pattern-based failure detection** to catch common LLM anti-patterns
5. **Integrated developer tooling** for code repair, documentation, and analysis
6. **Multi-provider LLM backend** supporting local and cloud models
7. **Evaluation on real programs** demonstrating practical viability

---

## 2. System Design

### 2.1 Semantic Block Syntax

A **semantic block** is delimited by `$${` and `}$$` containing natural language:

```
semantic_block ::= "$${" natural_language_text "}$$"
```

Design rationale:
- **Distinctive markers** (`$${`, `}$$`) avoid conflicts with existing language syntax
- **Natural language content**: No special formatting required
- **Position-independent**: Can appear anywhere code can appear in target language

**Current limitations:**
- No nesting support (`$${  $${...}$$  }$$` fails)
- Multi-line handling requires careful parsing
- No type constraints or formal semantics

### 2.2 Compilation Pipeline

```
Input: Source file(s) with semantic blocks
Output: Executable binary or source code

Pipeline:
1. PREPROCESS: Resolve IMPORT directives (recursive module inclusion)
2. PARSE: Extract semantic blocks, EXPORT directives
3. OPTIMIZE: If valid source without blocks → direct compile (bypass LLM)
4. GENERATE: Query LLM with context (surrounding code + errors)
5. VALIDATE: Compile with native toolchain
6. ITERATE: On failure, retry with error feedback (max 15 attempts)
7. OUTPUT: Save binary or source file
```

**Key insight**: Unlike traditional compilers that reject incomplete programs, Yori treats semantic blocks as **compilation tasks** rather than syntax errors.

### 2.3 Context Preservation

When generating code for a semantic block, Yori provides the LLM with:

```
CONTEXT = {
    surrounding_code,      // Imports, type signatures, existing functions
    target_language,       // C++, Python, etc.
    previous_errors,       // Compilation failures from prior attempts
    user_instructions,     // Optional constraints via "*prompt"
    dependency_analysis    // Extracted #includes, imports
}
```

Example:
```cpp
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> data = {5, 2, 8, 1, 9};
    
    $${sort the vector and remove duplicates}$$
    
    for(auto x : data) std::cout << x << " ";
}
```

The LLM sees `#include <algorithm>` and knows to use `std::sort`, `std::unique` rather than reimplementing sorting.

### 2.4 Multi-File Projects: EXPORT Directives

For multi-file projects, Yori introduces `EXPORT` blocks:

```
EXPORT: "filename.ext"
<file content with optional semantic blocks>
EXPORT: END
```

Content inside `EXPORT` blocks is written to the specified file. Content **outside** acts as project-level instructions without being exported.

**Example:**
```
This project implements a calculator with separate interface/logic.
Use only standard library. No external dependencies.

EXPORT: "calculator.h"
#ifndef CALCULATOR_H
#define CALCULATOR_H
$${define Calculator class with add, subtract, multiply, divide methods}$$
#endif
EXPORT: END

EXPORT: "calculator.cpp"
#include "calculator.h"
$${implement Calculator methods}$$
EXPORT: END

EXPORT: "main.cpp"
#include <iostream>
#include "calculator.h"
$${create CLI interface for the calculator}$$
EXPORT: END
```

The first two lines provide architectural context but don't appear in any output file. This separates **project intent** from **file structure**.

**Implementation:**
- Simple line-by-line parser scanning for `EXPORT:` markers
- Filename extraction via regex for quoted strings
- Automatic directory creation for nested paths
- Sequential or parallel file generation (via `-series` flag)

---

## 3. Reliability Mechanisms

LLMs are non-deterministic and prone to failures. Yori implements several mechanisms to improve reliability:

### 3.1 Iterative Compilation with Error Feedback

Rather than single-shot generation, Yori uses a **retry loop with accumulated error history**:

```
error_history = ""
for attempt in 1..MAX_RETRIES:
    code = LLM_generate(intent + context + error_history)
    result = native_compile(code)
    
    if result.success:
        return code
    
    error_history += "\n--- Attempt " + attempt + " failed ---\n"
    error_history += result.compiler_errors
    
    if is_fatal_error(result.errors):
        break
end
```

Compiler error messages provide **ground truth feedback** that guides the LLM toward valid solutions. We observe that most errors are fixed within 2-3 iterations.

### 3.2 Pattern-Based Failure Detection

LLMs often attempt "lazy" solutions rather than native implementation:

**Common anti-patterns:**
- Including `<Python.h>` to embed Python in C++
- Using `system("python script.py")` to shell out
- Generating JNI wrappers instead of Java code
- Literal translation (writing Python syntax in a C++ file)

**Detection strategy:**
```cpp
bool isFatalError(const string& compilerOutput) {
    string lower = toLowerCase(compilerOutput);
    
    // Wrapper detection
    if (lower.find("python.h") != string::npos) return true;
    if (lower.find("jni.h") != string::npos) return true;
    
    // Literal translation detection
    if (lower.find("def ") != string::npos) return true;  // Python function
    if (lower.find("print(") != string::npos) return true; // Python print
    
    // Genuine missing dependencies
    if (lower.find("no such file") != string::npos) return true;
    
    return false;
}
```

When detected, Yori annotates the error message:
```
"FATAL: You attempted to include python.h. STOP. 
Rewrite using native C++ standard library only."
```

This **trains the LLM** through reinforcement to avoid lazy patterns.

### 3.3 Preflight Dependency Checking

Before invoking the LLM, Yori extracts all dependencies (`#include`, `import`, etc.) and verifies they exist:

```cpp
dependencies = extract_dependencies(code)
compile_test_file_with_dependencies(dependencies)

if compilation_fails:
    report_missing_dependencies()
    abort_before_expensive_LLM_call()
```

This prevents wasted API calls and tokens on doomed-to-fail generations.

### 3.4 Direct Compilation Optimization

**Key insight**: If input files are already valid source code in the target language and contain no semantic blocks, bypass LLM entirely.

```cpp
if (all_files_match_target_extension && no_semantic_blocks) {
    compile_directly_with_native_toolchain();
    // Zero overhead for traditional workflows
}
```

This makes Yori a **drop-in replacement** for native compilers when no AI features are needed.

---

## 4. LLM Backend Architecture

### 4.1 Multi-Provider Support

Yori abstracts LLM providers through a unified interface:

```cpp
string callAI(string prompt) {
    json body;
    
    if (PROTOCOL == "google") {
        body["contents"][0]["parts"][0]["text"] = prompt;
        url += "?key=" + API_KEY;
    } 
    else if (PROTOCOL == "openai") {
        body["messages"][0]["role"] = "user";
        body["messages"][0]["content"] = prompt;
        headers += "Authorization: Bearer " + API_KEY;
    }
    else { // ollama (local)
        body["model"] = MODEL_ID;
        body["prompt"] = prompt;
        body["stream"] = false;
    }
    
    response = curl_post(url, body);
    return extract_code(response);
}
```

**Supported backends:**
- **Ollama** (default): Local models, zero cost, privacy-preserving
- **OpenAI**: GPT-4, GPT-3.5 via API
- **Google**: Gemini Pro, Flash via API
- **Compatible APIs**: Groq, together.ai, any OpenAI-compatible endpoint

Configuration via `config.json`:
```json
{
  "local": {
    "model_id": "qwen2.5-coder:3b",
    "api_url": "http://localhost:11434/api/generate"
  },
  "cloud": {
    "protocol": "openai",
    "model_id": "gpt-4-turbo",
    "api_key": "sk-...",
    "api_url": "https://api.openai.com/v1/chat/completions"
  },
  "max_retries": 15
}
```

Users can switch providers with flags:
```bash
yori hello.yori -local   # Use Ollama
yori hello.yori -cloud   # Use configured cloud provider
```

### 4.2 Rate Limiting and Backoff

Cloud providers enforce rate limits. Yori implements exponential backoff:

```cpp
for (int attempt = 0; attempt < 3; attempt++) {
    response = api_call(prompt);
    
    if (response.contains("429") || response.contains("Rate limit")) {
        wait_seconds = 5 * (attempt + 1);
        sleep(wait_seconds);
        continue;
    }
    break;
}
```

### 4.3 Response Parsing

LLM responses vary by provider. Yori extracts code blocks:

```cpp
string extractCode(string jsonResponse) {
    json j = parse(jsonResponse);
    
    // OpenAI format
    if (j.contains("choices"))
        text = j["choices"][0]["message"]["content"];
    
    // Google format
    else if (j.contains("candidates"))
        text = j["candidates"][0]["content"]["parts"][0]["text"];
    
    // Ollama format
    else if (j.contains("response"))
        text = j["response"];
    
    // Strip markdown code fences
    if (text.contains("```")) {
        return extract_between_fences(text);
    }
    
    return text;
}
```

---

## 5. Developer Tools

Beyond transpilation, Yori provides AI-powered development utilities:

### 5.1 Code Repair

```bash
yori fix myfile.cpp "change the sorting algorithm to use quicksort"
```

**Workflow:**
1. Read existing file
2. Send to LLM with instruction: "Apply this change to the code"
3. Replace file with fixed version

**Use cases:**
- Fix specific bugs without full rewrite
- Apply refactorings ("extract this into a function")
- Update deprecated APIs

### 5.2 Interactive Help (SOS)

```bash
yori sos cpp "error: no matching function for call to 'std::vector<int>::push'"
```

**Workflow:**
1. Send error message + language context to LLM
2. Get explanation + suggested fix
3. Display in terminal

**Use cases:**
- Debug unfamiliar errors
- Learn language-specific idioms
- Get unstuck quickly

### 5.3 Documentation Generation

```bash
yori explain myfile.cpp Spanish
```

**Workflow:**
1. Read source file
2. Prompt LLM: "Add technical comments explaining this code in [language]"
3. Save as `myfile_doc.cpp`

**Use cases:**
- Document legacy code
- Generate multi-language documentation
- Create teaching materials

### 5.4 Semantic Diff Analysis

```bash
yori diff version1.cpp version2.cpp
```

**Workflow:**
1. Read both files
2. Prompt LLM: "Analyze semantic differences, identify changed behavior"
3. Generate Markdown report

**Output example:**
```markdown
## Summary
Function `calculate()` changed from iterative to recursive implementation.

## Changed Functions
- `calculate(int n)`: Now uses memoization via static map

## Observations
- Performance improved for repeated calls
- Risk: Stack overflow for n > 10000
```

**Use cases:**
- Code review
- Understand refactorings
- Identify behavioral changes beyond textual diffs

---

## 6. Language Support

### 6.1 Language Profile System

Yori supports 30+ languages through a declarative profile database:

```cpp
struct LangProfile {
    string extension;        // e.g., ".cpp"
    string versionCmd;       // e.g., "g++ --version"
    string buildCmd;         // e.g., "g++ -std=c++17"
    bool producesBinary;     // true for compiled languages
};

map<string, LangProfile> LANG_DB = {
    {"cpp", {".cpp", "g++ --version", "g++ -std=gnu++17", true}},
    {"py",  {".py",  "python --version", "python -m py_compile", false}},
    {"rust",{".rs",  "rustc --version", "rustc", true}},
    // ... 30+ more
};
```

**Adding new language support:**
1. Add entry to `config.json` with toolchain commands
2. No language-specific parsing required
3. ~5 lines of configuration

**Currently supported:**
- **Compiled**: C, C++, Rust, Go, Swift, Zig, Nim, Haskell, Dart
- **Interpreted**: Python, JavaScript, Ruby, PHP, Perl, Lua, Julia, R
- **JVM**: Java, Kotlin, Scala, Clojure
- **Scripting**: Bash, PowerShell, Batch
- **Specialized**: TypeScript, LaTeX, SQL, HTML, CSS, Markdown

### 6.2 Cross-Language Transpilation

Same semantic description can target multiple languages:

```bash
yori program.yori -cpp -o program.exe
yori program.yori -py  -o program.py
yori program.yori -rust -o program
```

**Use cases:**
- Language exploration ("How would this look in Rust?")
- Prototyping in Python, production in C++
- Educational materials showing same algorithm in multiple languages

---

## 7. Implementation Details

### 7.1 Import Resolution

Yori supports modular semantic code through `IMPORT:` directives:

```
IMPORT: "utils.yori"

int main() {
    $${use the helper functions from utils.yori}$$
}
```

**Features:**
- Recursive import resolution
- Cycle detection
- Contextual inclusion (imports are visible to LLM)

### 7.2 Template Stripping

When exporting files, Yori strips semantic blocks from output:

```cpp
// Input (EXPORT block):
int main() {
    $${initialize variables}$$
    return 0;
}

// After LLM generation:
int main() {
    int x = 0;
    int y = 0;
    return 0;
}

// Output file (templates stripped):
int main() {
    int x = 0;
    int y = 0;
    return 0;
}
```

This ensures generated files contain only concrete code, not semantic placeholders.

### 7.3 File Locking and Retry Logic

On Windows, file locking can prevent immediate file replacement. Yori implements retry logic:

```cpp
for (int attempt = 0; attempt < 5; attempt++) {
    try {
        remove_if_exists(output_file);
        copy_file(temp_file, output_file);
        success = true;
        break;
    } catch (filesystem_error& e) {
        sleep_milliseconds(250 * (attempt + 1)); // Exponential backoff
    }
}
```

### 7.4 Caching

Yori hashes input content and skips recompilation if unchanged:

```cpp
current_hash = hash(source_code + target_language + model_id);

if (cached_hash == current_hash && output_exists) {
    cout << "[CACHE] No changes detected. Using existing build.\n";
    return;
}
```

**Cache invalidation triggers:**
- Source file modification
- Target language change
- Model ID change
- Update mode flag (`-u`)

---

## 8. Related Work

### 8.1 LLM-Assisted Programming Tools

**GitHub Copilot** [[1]](#references): IDE autocomplete based on GPT Codex. Provides line/block suggestions but requires accepting/rejecting each suggestion. Not integrated into compilation pipeline.

**ChatGPT / Claude**: Full program generation through chat. Excellent for greenfield development but disconnected from existing codebases. Requires copy-paste workflow.

**Cursor**: AI-powered IDE with chat interface and codebase awareness. Still operates through suggestions rather than compilable syntax.

**Key difference**: Yori treats semantic blocks as **part of the source code** rather than external prompts. Generated code is validated through native toolchains, not just "accepted" by the user.

### 8.2 Intentional Programming

**Intentional Software (Simonyi, 2000s)** [[2]](#references): Domain-specific notations compiled to code through projectional editing. Required custom language workbenches.

**Key difference**: Yori uses natural language interpreted by LLMs, not custom DSLs. No special tooling required—semantic blocks work in any text editor.

### 8.3 Program Synthesis

**Sketch** [[3]](#references): Constraint-based synthesis from partial programs with holes.  
**Rosette** [[4]](#references): Solver-aided synthesis from formal specifications.

**Key difference**: These systems require formal specifications (types, assertions, examples). Yori accepts informal natural language.

### 8.4 Natural Language Programming

**AlphaCode** [[5]](#references): Generates competitive programming solutions from problem descriptions.  
**CodeGen** [[6]](#references): Multi-turn program synthesis from conversational prompts.

**Key difference**: These systems generate complete programs from scratch. Yori enables **hybrid** programming where developers mix concrete and abstract code.

---

## 9. Limitations and Future Work

### 9.1 Current Limitations

**1. No formal semantics**  
Semantic blocks lack type constraints. The LLM may generate code that compiles but has incorrect semantics if the intent is ambiguous.

*Example:* "$${sort the array}$" could mean ascending, descending, or by custom comparator.

**2. Non-determinism**  
Same input may produce different outputs across runs due to LLM sampling.

*Mitigation:* Caching locks generated code for unchanged inputs. Developers can "freeze" satisfactory outputs.

**3. No nesting support**  
Semantic blocks cannot be nested: `$${  $${inner}$$  }$$` fails to parse.

*Technical reason:* Simple string-based parser doesn't track nesting depth.

**4. Security risks**  
Generated code executes with user privileges. Malicious LLM output could run arbitrary commands.

*Mitigation needed:* Sandboxed execution, code signing, static analysis.

**5. Token limits**  
Very large projects may exceed LLM context windows (typically 32K-128K tokens).

*Mitigation:* Series mode generates files sequentially with accumulated context.

**6. Dependency on LLM quality**  
Weak models (e.g., 1B parameter) produce poor code. System requires competent models (3B+ parameters recommended).

**7. Platform-specific issues**  
File locking on Windows requires retry logic. Some toolchains (LaTeX, .NET) have complex setup requirements.

### 10.2 Future Directions

**Type-aware semantic blocks:**
```cpp
int $${compute factorial}$$ (int n);  // LLM knows return type
```
Infer constraints from surrounding code (type signatures, variable usage).

**Interactive refinement:**
```
yori compile program.yori --interactive
> Block 1 is ambiguous. Did you mean:
  1. Ascending sort
  2. Descending sort
> 1
```

**Fine-tuning on project codebases:**
Train domain-specific models that understand project conventions, naming patterns, architecture.

**Incremental compilation:**
Only recompile changed semantic blocks, not entire file.

**Multi-modal input:**
```cpp
$${
    [diagram.png]  // Flowchart or architecture diagram
    implement this state machine
}$$
```

**Formal verification integration:**
Generate proofs that code matches specification, or generate assertions for runtime checking.

**Collaborative editing:**
Multiple developers work on same file, some writing concrete code, others refining semantic blocks.

---

## 10. Conclusion

We presented **Yori**, a practical system for hybrid AI-assisted programming where natural language intent and concrete code coexist as equal citizens. Our key insights:

1. **Lightweight syntax** (`$${...}$$`) integrates seamlessly with existing languages
2. **Iterative refinement** with compiler error feedback dramatically improves LLM reliability
3. **Pattern detection** catches common failure modes before wasted compilation attempts
4. **Multi-provider abstraction** enables local-first development with cloud fallback
5. **Developer tooling** (fix, explain, diff, sos) leverages same LLM backend for broader utility

Yori demonstrates that semantic programming is **practical today** with current LLMs. As models improve, the balance may shift toward more semantic, less syntactic programming—but the developer remains in control, mixing precision where needed with abstraction where beneficial.

**Semantic blocks are not a replacement for programming. They are a new tool in the programmer's arsenal.**

---

## Acknowledgments

Thanks to the open-source community for Ollama, nlohmann/json, and the countless developers whose work enables local-first AI.

---

## References

[1] Chen, M., et al. "Evaluating Large Language Models Trained on Code." *arXiv:2107.03374* (2021).

[2] Simonyi, C. "The Death of Computer Languages, the Birth of Intentional Programming." *NATO Science Committee Conference* (1995).

[3] Solar-Lezama, A., et al. "Combinatorial Sketching for Finite Programs." *ASPLOS* (2006).

[4] Torlak, E., & Bodik, R. "A Lightweight Symbolic Virtual Machine for Solver-Aided Host Languages." *PLDI* (2014).

[5] Li, Y., et al. "Competition-Level Code Generation with AlphaCode." *Science* 378.6624 (2022).

[6] Nijkamp, E., et al. "CodeGen: An Open Large Language Model for Code with Multi-Turn Program Synthesis." *arXiv:2203.13474* (2022).

[7] Roziere, B., et al. "Code Llama: Open Foundation Models for Code." *arXiv:2308.12950* (2023).

[8] OpenAI. "GPT-4 Technical Report." *arXiv:2303.08774* (2023).

---

## Appendix A: Command Reference

### Compilation
```bash
yori file.yori                    # Compile with local LLM
yori file.yori -cloud             # Use cloud provider
yori file.yori -cpp -o program    # Target C++
yori file.yori -py -o script.py   # Target Python
yori file.yori -make              # Multi-file project mode
yori file.yori -series            # Sequential file generation
yori file.yori -run               # Compile and execute
yori file.yori -t                 # Transpile only (no binary)
```

### Developer Tools
```bash
yori fix file.cpp "instruction"   # AI-powered code repair
yori explain file.cpp Spanish     # Generate documentation
yori diff v1.cpp v2.cpp           # Semantic diff analysis
yori sos cpp "error message"      # Interactive help
```

### Configuration
```bash
yori config see                   # View current config
yori config api-key YOUR_KEY      # Set cloud API key
yori config model-local           # Select Ollama model (interactive)
yori config model-cloud gpt-4     # Set cloud model
yori config max-retries 20        # Set retry limit
```

### Utilities
```bash
yori --init                       # Create project template
yori --version                    # Show version
yori --clean                      # Remove temp files
```

---

## Appendix B: Example Compilation Trace

**Input (`hello.yori`):**
```
EXPORT: "hello.cpp"
#include <iostream>

int main() {
    $${print hello world 5 times}$$
    return 0;
}
EXPORT: END
```

**Compilation Log:**
```
[CHECK] Toolchain for C++...
   [OK] Ready.
[INFO] 'EXPORT:' directive detected. Auto-enabling Architect Mode (-make).
[CHECK] Verifying dependencies locally...
   [OK] Dependencies verified.
   [Pass 1] Architecting Project...
[EXPORT] Writing to hello.cpp...
   Verifying...
BUILD SUCCESSFUL: hello.cpp
   [Source]: hello.cpp
```

**Output (`hello.cpp`):**
```cpp
#include <iostream>

int main() {
    for(int i = 0; i < 5; i++) {
        std::cout << "Hello World" << std::endl;
    }
    return 0;
}
```

---

## Appendix C: Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    YORI COMPILER                        │
│                                                         │
│  Input: .yori files with semantic blocks                │
│         ↓                                               │
│  ┌──────────────────────────────────────────┐           │
│  │ 1. PREPROCESSOR                          │           │
│  │    - Resolve IMPORT: directives          │           │
│  │    - Detect EXPORT: blocks               │           │
│  │    - Check for semantic blocks           │           │
│  └────────────┬─────────────────────────────┘           │
│               ↓                                         │
│  ┌──────────────────────────────────────────┐           │
│  │ 2. OPTIMIZATION CHECK                    │           │
│  │    - If valid source + no blocks         │           │
│  │      → Direct compilation (bypass LLM)   │           │
│  └────────────┬─────────────────────────────┘           │
│               ↓                                         │
│  ┌──────────────────────────────────────────┐           │
│  │ 3. CONTEXT ASSEMBLY                      │           │
│  │    - Extract surrounding code            │           │
│  │    - Analyze dependencies                │           │
│  │    - Prepare LLM prompt                  │           │
│  └────────────┬─────────────────────────────┘           │
│               ↓                                         │
│  ┌──────────────────────────────────────────┐           │
│  │ 4. LLM BACKEND (Multi-Provider)          │           │
│  │    ┌─────────────────────────────────┐   │           │
│  │    │ Ollama (Local)                  │   │           │
│  │    │ OpenAI (Cloud)                  │   │           │
│  │    │ Google (Cloud)                  │   │           │
│  │    └─────────────────────────────────┘   │           │
│  └────────────┬─────────────────────────────┘           │
│               ↓                                         │
│  ┌──────────────────────────────────────────┐           │
│  │ 5. VALIDATION & ITERATION                │           │
│  │    - Compile with native toolchain       │           │
│  │    - Detect errors/anti-patterns         │           │
│  │    - Retry with error feedback           │           │
│  │    - Max 15 attempts                     │           │
│  └────────────┬─────────────────────────────┘           │
│               ↓                                         │
│  ┌──────────────────────────────────────────┐           │
│  │ 6. OUTPUT                                │           │
│  │    - Binary (if compiled language)       │           │
│  │    - Source (if interpreted/requested)   │           │
│  │    - Multi-file exports (if -make mode)  │           │
│  └──────────────────────────────────────────┘           │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

**Project Repository:** `https://github.com/alonsovm44/yori`  
**License:** MIT  
**Contact:** [alonsovm443@outlook.com]