# gwatch
Author: Konrad Kaczmarczyk

Small utility to monitor global variables and execute a command when they change.

## Requirements
POSIX-compatible shell
make
C++ compiler (g++/clang) — if the project is implemented in C/C++
program monitored should be typical ELF64 executable not striped

## Build
From the repository root:

```
make
```

This builds the gwatch executable (or the primary target defined by the project Makefile).

## Usage
Basic usage:

```
./gwatch --var <global_variable> --exec <command> [-- arg1 arg2 ... argN]
```

### Options:

- --var Monitor the global variable named
- --exec Execute when the watched variable changes
- [-- arg1 arg2 ... argN] List of arguments passed to executable

## Example
I prepared a example program called `sample` so utility can be run with

```
./gwatch --var global_variable --exec ./sample
```

## Tests
Install gtests (i wasn't able to add it correctly as a submodule)
```
git clone https://github.com/google/googletest.git third_party/googletest
```

Run the test suite with:

```
make test
```

## Comment
I wasn't able to meet overhead criteria, but i hope it doesn't disqualify my solution.
I assumed program needed to be responsive which requires constant printing to terminal which is slow.
