# USH — A Lightweight Unix-Like Shell in C

<img width="480" height="240" alt="image" src="https://github.com/user-attachments/assets/51002c2e-5235-4a25-a761-3d35a41c7c24" />


**USH (Uday Shell)** is a minimal, extensible Unix-style shell written in **C**.  
It supports interactive and batch modes, command parsing, process control, built-in commands, and pipeline execution, built from scratch with custom data structures and a modular architecture.

> Developed as a deep dive into systems programming and process management,  
> WSH recreates the core logic of a working shell environment, from parsing to process forking and I/O redirection.

---

## ⚙️ Key Features

- **Interactive & Batch Execution** — runs user commands or scripts seamlessly.  
- **Built-in Commands:**  
  `exit`, `alias`, `unalias`, `which`, `path`, `cd`, and `history`.  
- **External Command Execution** using `fork()`, `execv()`, and `waitpid()`.  
- **Pipeline Support** — run up to 128 commands with `|` redirection (e.g., `ls -l | grep .c | wc -l`).  
- **Dynamic Memory Utilities** — custom implementations of:
  - `dynamic_array` for command tokens
  - `hash_map` for alias storage and lookups  
- **Robust Error Handling** with `perror()` and graceful recovery.  
- **Optimized Build System** with `make` targets for release/debug modes.

---

## 🧠 Architecture Overview

- **`wsh.c`** — main shell loop handling input parsing, process creation, and command execution.  
- **`dynamic_array.c/h`** — custom resizable array implementation for storing parsed tokens dynamically.  
- **`hash_map.c/h`** — key–value store used for alias handling and command lookups.  
- **`utils.c/h`** — helper functions for string operations, error management, and input sanitation.  
- **`Makefile`** — build automation with optimized (`wsh`) and debug (`wsh-dbg`) targets.  
- **`build/`** — contains compiled object files and separate directories for:  
  - `release/` — optimized binaries  
  - `debug/` — debug builds with symbols  
- **`wsh`** — compiled release binary.  
- **`wsh-dbg`** — compiled debug binary.  


Each module is isolated for clarity and reusability, allowing the shell to scale from command parsing to multiprocess pipelines.
