# Gerp

A command-line file search engine — a mini `grep` — that recursively indexes every word in a directory tree and answers instant lookup queries against a hand-built hash table, with case-insensitive search, full source provenance (file + line number) on every hit, and switchable output destinations mid-session.

## Tech Stack

- **C++** (C++11, compiled with `clang++`) — standard library only, no external dependencies
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

**Prerequisites:** `clang++` (or another C++11-compatible compiler) and `make`, **on Linux x86-64**.

> **Platform note:** `DirNode.o`/`FSTree.o` are precompiled **Linux ELF x86-64** object files (see Architecture note below) — they cannot be linked against macOS (Mach-O) or Windows object files. On macOS/Windows, build inside a Linux container instead:
> ```bash
> docker run --rm -it -v "$PWD":/src -w /src --platform linux/amd64 gcc:13 bash
> apt-get update && apt-get install -y clang
> make
> ```

```bash
git clone https://github.com/anontriton/gerp.git
cd gerp
make
```

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

> **Note:** `DirNode.o`/`FSTree.o` are precompiled objects with no available source (see [CHANGELOG.md](CHANGELOG.md)) — they're checked into this repo since they can't be rebuilt from scratch, and the Makefile links against them with `-no-pie` for compatibility with modern toolchains.

## Future Roadmap / Enhancements

- [ ] **Priority:** Reimplement `DirNode`/`FSTree` from source (the interface is fully documented in `DirNode.h`/`FSTree.h`) to drop the dependency on precompiled, Linux-only object files — this is what currently blocks building natively on macOS/Windows
- [ ] Add automated unit tests (currently verified via manual/scripted runs — see CHANGELOG for the verification pass)
- [ ] Support multi-word / phrase queries instead of single tokens
- [ ] Add regex or wildcard search support
- [ ] Replace the custom hash table with a benchmarked comparison against `std::unordered_map` to justify (or drop) the hand-rolled version
- [ ] Improve error handling for unreadable files/directories during indexing
- [ ] Make `@f` accept the filename on the same line as the command, instead of requiring a second line of input

## Demo / Screenshots

<!-- Add a terminal recording or screenshot here showing a sample indexing run and a few queries, e.g. docs/demo.gif or docs/screenshot.png -->
