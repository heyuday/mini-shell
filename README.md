# WSH – A Lightweight Unix Shell in C

**WSH** (Wisconsin Shell) is a minimal Unix-like shell built entirely in C.  
It supports execution of built-in commands, external processes, and pipelines — implementing core OS concepts like process control, system calls, and I/O redirection.

## 🚀 Features

- Custom command parser supporting arguments, quotes, and pipes
- Execution of both built-ins and external programs via `fork()` and `execvp()`
- Dynamic arrays and hash maps implemented from scratch
- Minimal memory management utilities (`malloc` wrappers, cleanup handlers)
- Comprehensive test suite with automated scripts

## 🧠 Core Files

| File                | Description                                 |
| ------------------- | ------------------------------------------- |
| `wsh.c`             | main shell loop and command execution       |
| `dynamic_array.c/h` | dynamic memory-safe array implementation    |
| `hash_map.c/h`      | custom hash map for environment variables   |
| `utils.c/h`         | helper functions for parsing and string ops |
| `tests/`            | unit and integration tests                  |

## 🧩 Build and Run

```bash
make
./wsh
```
