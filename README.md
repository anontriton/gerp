# Gerp

A command-line file search engine — a mini `grep` — that recursively indexes every word in a directory tree and answers instant lookup queries against a hand-built hash table, with case-insensitive search, full source provenance (file + line number) on every hit, and switchable output destinations mid-session.

## Tech Stack

- **C++11** — standard library plus POSIX `dirent`/`stat` for directory walking; no external dependencies
- Custom **hash table** implementation (separate chaining, dynamic resizing)
- `make` build system

## Architecture

```
main.cpp
  └── FileManager            interactive query loop, output redirection
        ├── FSTree / DirNode recursive filesystem representation (indexes the tree)
        ├── HashTable  ht       exact-case word index
        ├── HashTable  htLower  lowercase word index (case-insensitive search)
        └── stringProcessing    word normalization (strip surrounding punctuation)
```

`FSTree` walks the target directory once at startup and builds an in-memory `DirNode` tree (each node holding its own files and child directories). `FileManager` then does a second pass over that tree, tokenizing every line of every file into words and inserting each one into **two** hash tables — one keyed by exact casing, one keyed by the lowercased form — so case-sensitive and case-insensitive queries never contaminate each other's results while both still return every occurrence of a word across the whole tree.

## Key Technical Features

- **Recursive directory indexing** — walks the `FSTree`/`DirNode` representation of the filesystem, reading every file line-by-line and tokenizing it into words.
- **Hand-rolled hash table** (`HashTable.cpp`) using separate chaining (`std::list<std::pair<string, string>>` buckets) for collision handling, with automatic **rehashing** (bucket count doubles) once the load factor crosses 0.7.
- **Dual-index case handling** — an exact-case table and a dedicated lowercase table, so `@i`/`@insensitive` queries correctly aggregate every casing of a word (`fox`, `Fox`, `FOX`, ...) without ever leaking into plain case-sensitive lookups.
- **Every occurrence is retained** — a word appearing 50 times across a directory tree returns all 50 hits, not just the most recent one, via accumulated newline-delimited records per key.
- **Interactive query loop** — `FileManager::processQueries` supports:
  - Plain word queries (case-sensitive by default)
  - `@i <word>` / `@insensitive <word>` for case-insensitive lookups
  - `@f <file>` (filename on the *next* input line) to redirect output mid-session
  - `@q` / `@quit` to exit
- **Token normalization** — `stripNonAlphaNum` trims leading/trailing punctuation from words before indexing/searching, so results match regardless of surrounding punctuation.
- **Search results include full provenance** — every hit resolves to `file/path:lineNumber: full line text`, so a query points directly at its source.

## Local Setup & Installation

**Prerequisites:** a C++11 compiler and `make`. Nothing else to install.

### macOS

```bash
xcode-select --install     # installs clang + make, if not already present
git clone https://github.com/anontriton/gerp.git
cd gerp
make
```

### Linux

```bash
sudo apt install build-essential   # Debian/Ubuntu; or: sudo dnf install gcc-c++ make
git clone https://github.com/anontriton/gerp.git
cd gerp
make
```

The build uses whichever compiler your platform ships (`c++` on macOS, `g++` on Linux). To pick one explicitly: `make CXX=clang++`.

Run it against a directory you want to index, writing results to an output file:

```bash
./gerp path/to/directory output.txt
```

Then type queries at the `Query?` prompt:

```
Query? fox
Query? @i fox
Query? @insensitive fox
Query? @f
newoutput.txt
Query? world
Query? @q
```

No environment variables are required — the tool only reads local files.

## Future Roadmap / Enhancements

- [ ] Add automated unit tests (currently verified via manual/scripted runs — see CHANGELOG for the verification pass)
- [ ] Windows support — directory walking uses POSIX `dirent`/`stat`; porting `FSTree` to `std::filesystem` (C++17) would make it build there too, and simplify the code
- [ ] Sort or rank results (currently returned in indexing order)
- [ ] Support multi-word / phrase queries instead of single tokens
- [ ] Add regex or wildcard search support
- [ ] Replace the custom hash table with a benchmarked comparison against `std::unordered_map` to justify (or drop) the hand-rolled version
- [ ] Improve error handling for unreadable files/directories during indexing
- [ ] Make `@f` accept the filename on the same line as the command, instead of requiring a second line of input

## Demo / Screenshots

<!-- Add a terminal recording or screenshot here showing a sample indexing run and a few queries, e.g. docs/demo.gif or docs/screenshot.png -->
