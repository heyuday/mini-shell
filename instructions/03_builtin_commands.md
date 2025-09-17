# Built-in Commands

Built-in commands (or "builtins") are commands that are implemented directly
within the shell itself, rather than as separate executable programs. When
you type a built-in command, the shell handles it internally without
creating a new process. This is different from external commands like `cp` or
`grep`, which are separate programs that the shell must find and execute.

Your built-in commands should return `1` if they fail / have errors, and `0`
on success. Read specific error handling instructions for each built-in command
below.

> [!tip]
>
> You might want to consider using `EXIT_SUCCESS` (0) and `EXIT_FAILURE` (1)
> macros defined in `stdlib.h` for better readability.

Here is a list of built-in commands that you must implement in your shell:

--------------------------------------------------------------------------------

## Exit (`exit`)

The `exit` command terminates the shell. When the user types `exit`, the shell
should free any allocated memory and exit cleanly.
The return code of `wsh` is the exit value (for processes) or the return
value (for builtins) of the **last command**. For simplicity, we are only
dealing with two return codes, `0` and `1`. A return code of `0` indicates
success, while a return code of `1` indicates failure.

**Example Usage:**

This is how you use the `exit` bulit-in command in interactive mode:

```shell
wsh> exit
```

### Handling Errors

- It is an error to provide any arguments to `exit`. If the user types
  `exit` followed by any arguments (e.g., `exit now`), your shell should print
  the error message `Incorrect usage of exit. Too many arguments\n` to `stderr`.
  `wsh` should not exit in this case.

--------------------------------------------------------------------------------

## Alias (`alias`)

The `alias` command allows users to create shortcuts for longer commands
(and their options). When the user types `alias name = command`, the shell
should store this alias. Later, when the user types `name` in the beginning
of a command, the shell should replace `name` with `command` before
executing it.

If `command` contains spaces, it should be enclosed in single quotes
(`'`) when creating the alias. The single quotes are not part of the command
itself; they are just used to help the parser understand where the command
starts and ends.

If a name is already defined as an alias, the new definition should
overwrite the previous one.

The command can be empty but the name cannot be empty. The command can have
spaces but the name cannot.

Typing alias by itself should print all currently defined aliases **sorted
ascending** by the `name`, one per line, in the format `name = 'command'`.

> [!important]
>
> The alias should only be replaced if it appears as the first token in the
> command.
>
> For example, if the user creates an alias `ll` for `ls -l`, then typing `ll`
> should execute `ls -l`. However, typing `echo ll` should not replace `ll`
> with `ls -l`.
>
> You might want to come back and check your alias implementation when you
> are working on pipelines and command substitution. Only the first token of
> each pipeline segment and the first token inside a sub-command of
> command substitution should be checked for alias replacement.

**Example Usage:**

```shell
wsh> alias ll = 'ls -l' # command contains spaces, so use single quotes
wsh> ll
(output of executing ls -l) # will differ based on files in the directory
wsh> alias
ll = 'ls -l'
wsh> alias e = echo
wsh> e ll # ll should not be replaced here
ll
wsh> alias e = exit # should overwrite previous alias for e
wsh> alias g = grep
wsh> alias # should print aliases sorted by name
e = 'exit'
g = 'grep'
ll = 'ls -l'
wsh> e
```

### Handling Errors

- If the user provides an incorrect format for the alias command, print the
  error message `Incorrect usage of alias. Correct format: alias name = 'command'\n`.
  - Examples of incorrect usage:
    - `alias ll 'ls -l'` (missing `=`)
    - `alias ll = ls -l` (missing single quotes around command with spaces)
    - `alias = 'ls -l'` (missing name)
  - `alias ll =` and `alias ll = ''` are acceptable because the command can be
    empty


--------------------------------------------------------------------------------


## Unalias (`unalias`)

The `unalias` command removes an existing alias. When the user types
`unalias name`, the shell should delete the alias for `name`. If `name` is
not defined as an alias, this command has no effect (not an error).

**Example Usage:**

```shell
wsh> alias ll = 'ls -l'
wsh> alias
ll = 'ls -l'
wsh> unalias ll
wsh> alias # should not print anything as there are no aliases
wsh> unalias hello # no error, even though 'hello' alias does not exist
```

### Handling Errors

- If the user provides an incorrect format for the unalias command, print the
  error message `Incorrect usage of unalias. Correct format: unalias name\n`.
  - Examples of incorrect usage:
    - `unalias` (missing name)
    - `unalias ll extra` (too many arguments)

--------------------------------------------------------------------------------

## Which (`which`)

The `which` command lets you ask the shell what command will be executed
when you give a specific command as input.

When the user types `which name`, the shell should first check if the `name`
is defined as an alias. If it is, the shell should output `name: aliased to 
'command'\n`, where `command` is the command associated with the alias. Notice
the command is wrapped in single quotes as it might have spaces in between.

If `name` is not an alias, the shell should then check if a builtin command
with that name exists. If it does, the shell should output `name: wsh 
builtin\n`.

If `name` is neither an alias nor a builtin, the shell should search for an
executable file named `name`. You need to handle both absolute/relative path
and [searching in path](02_executing_external_commands.md#searching-in-path).

If the relative or absolute path exists and is executable, print `name: 
found at {path}` (something like `echo: found at `/bin/echo`). 

If the command is found in one of the directories specified by `PATH`, the
print statement will be same as if you above. If the command is not found, 
print the error message `name: not found\n`.

**Example Usage:**

```shell
wsh> alias ll = 'ls -l'
wsh> which ll
ll aliased to 'ls -l'
wsh> which alias
alias: wsh builtin
wsh> which cp
/bin/cp
wsh> which /bin/cp
/bin/cp
wsh> which ./myprog
./myprog
wsh> which nonexistentcommand
nonexistentcommand not found
```

> [!tip]
>
> You can probably reuse some of the code you wrote for searching commands in
> `PATH` in the [executing external commands](02_executing_external_commands.md)
> part of the project. You should handle cases where `PATH` is empty or set
> in the same way as described there.

### Handling Errors

- It is an error to provide more than one argument to `which`. If the user
  types `which` followed by zero or more than one argument (e.g., `which` or
  `which cp extra`), your shell should print the error message
  `Incorrect usage of which. Correct format: which name\n`.

--------------------------------------------------------------------------------

## Path (`path`)

The `path` command allows users to view and modify the `PATH` environment
variable, which is a colon-separated list of directories that the shell searches
for executable files. When the user types `path dir1:dir2:...:dirN`, the
shell should set the `PATH` variable to the specified string.

If the user types `path` with no arguments, the shell should print the
current value of the `PATH` variable.

**Example Usage:**

```shell
wsh> path
/bin
wsh> path /usr/bin:/bin
wsh> path
/usr/bin:/bin
```

### Handling Errors

- It is an error to provide more than one argument to `path`. If the user
  types `path` followed by more than one argument (e.g., `path /usr/bin /bin`),
  your shell should print the error message
  `Incorrect usage of path. Correct format: path dir1:dir2:...:dirN\n`.

--------------------------------------------------------------------------------

## Change Directory (`cd`)

The `cd` command changes the current working directory of the shell. When the
user types `cd directory`, the shell should change its current working
directory to `directory`.

If the user types `cd` with no arguments, the shell
should change the current working directory to the user's home directory, which
can be obtained from the `HOME` environment variable.

> [!tip]
>
> Look into `chdir()` function to change the current working directory and
> `getenv()` function to get the value of the `HOME` environment variable.

**Example Usage:**

```shell
wsh> pwd
/root # you run as root in the docker container by default
wsh> cd /tmp
wsh> pwd # an executable (if you have the which builtin, try `which pwd`)
/tmp
wsh> cd hello # assuming /tmp/hello exists
wsh> pwd
/tmp/hello
wsh> cd h1 # assuming /tmp/hello/h1 does not exist
cd: No such file or directory
wsh> pwd # should still be /tmp/hello
/tmp/hello
wsh> cd # should go to home directory
wsh> pwd
/root # you run as root in the docker container by default
```

### Handling Errors

- If the user provides an incorrect format for the `cd` command (i.e., more
  than one argument), print the error message
  `Incorrect usage of cd. Correct format: cd | cd directory\n`.
- If the home environment variable is not set when the user types `cd` with
  no arguments, print the error message `cd: HOME not set\n`.
- If the specified directory does not exist or is not a directory, print the
  error message `cd: No such file or directory\n`. This can be done with
  `perror("cd")` to include the system error message.

--------------------------------------------------------------------------------

## History (`history`)

The `history` command displays a list of previously executed commands in the
current shell session. When the user types `history`, the shell should print
all the commands that have been entered so far, one per line, in the order they
were executed.

When the user types `history n`, where `n` is a positive integer, the shell
should print the n<sup>th</sup> command from the history (1-based index).

> [!important]
>
> The history should store the commands as they were entered by the user,
> without any modifications (e.g., alias substitution, trimming spaces,
> command substitution, etc.).
>
> The history can grow quite large, so you should dynamically allocate
> memory for it.

**Example Usage:**

```shell
wsh> alias e = echo
wsh> e hello
hello
wsh> ls
(output of ls)
wsh> history
alias e = echo
e hello
ls
wsh> history 2
e hello
wsh> history 5
history 2
wsh> history 7 # this command is the 7th command in this session (not in 
history yet)
Invalid argument passed to history
wsh>
```

### Handling Errors

- If `n` is out of range (less than 1 or greater than the number of commands in
  history) or if the provided argument cannot be parsed to an integer, the shell
  should print the error message `Invalid argument passed to history\n`.
- It is an error to provide more than one argument to `history`. If the user
  types `history` followed by more than one argument (e.g., `history 2 extra`),
  your shell should print the error message
  `Incorrect usage of history. Correct format: history | history n\n`.
