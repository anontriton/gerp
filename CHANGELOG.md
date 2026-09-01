# Changelog

## Portfolio prep — build & functionality fixes

Before this pass, `gerp` did not compile on a modern toolchain, and even after
getting it to compile, running it and issuing search queries returned
`Not Found` for every single word, no matter what was actually in the indexed
files. The root causes turned out to be one build-environment issue and
seven separate bugs in the original submission. All are fixed and verified
below (build + a full functional pass in a Linux x86-64 container, matching
the original grading environment).

### Build

- **Added `-no-pie` to `LDFLAGS` in the `Makefile`.** `DirNode.o` and
  `FSTree.o` are precompiled instructor-provided object files with no
  corresponding source anywhere in this repo's history — they can't be
  recompiled, only relinked. They were built as non-PIE objects, and modern
  linkers default to producing PIE executables, so linking failed with
  `relocation R_X86_64_32 against '.rodata.str1.1' can not be used when
  making a PIE object`. `-no-pie` restores the old linking behavior these
  objects require.
- Updated `.gitignore` so its `*.o` rule doesn't also swallow `DirNode.o`/
  `FSTree.o` — they're committed binaries this project depends on, not
  build output.

### Search correctness (`FileManager.cpp`, `HashTable.cpp`)

1. **`FileManager::indexFiles` never read any file contents.** It called
   `ifstream` directly on `parentPath + "/" + currNode->getName()`, treating
   every directory node as if it *were* a file. It never called
   `DirNode::getFile(n)` to enumerate the actual files inside a directory, so
   no word was ever indexed — every query, on every directory, always came
   back `Not Found`. Rewrote it to iterate `numFiles()`/`getFile(n)` for the
   current directory and recurse into `numSubDirs()`/`getSubDir(n)` for
   children, matching the traversal pattern the project's own (unused)
   `FSTreeTraversal.cpp` reference already demonstrated.

2. **`HashTable::insert` overwrote instead of accumulating.** Inserting a
   second occurrence of an existing key replaced the first
   (`itr->second = value`), so only the *last* occurrence of any word in the
   whole indexed tree was ever findable. Changed it to append
   (`itr->second += "\n" + value`), so every occurrence of a word survives.

3. **`FileManager::processResults` re-parsed an already-formatted result
   string by splitting on whitespace** (`iss >> filePath >> lineNum >>
   line`), which silently failed the moment a matched line contained more
   than one word — i.e. almost always, for real text. This is why a search
   could succeed internally and still print nothing. Rewrote it to split
   accumulated results on newlines and locate the two delimiter colons by
   position, which correctly handles arbitrary spaces in the matched line.

4. **No actual case-insensitive index existed.** Words were indexed only
   under their original casing, so a lowercased case-insensitive query could
   only ever match a word that happened to already be all-lowercase in the
   source file — a differently-cased occurrence (e.g. `FOX` vs. `fox`) was
   invisible to `@i`/`@insensitive` search. Added a second `HashTable`
   (`htLower`) populated with the lowercase form of every word, and case
   sensitive vs. insensitive queries now route to the appropriate table.
   (An earlier attempt at fixing this by inserting a lowercase duplicate key
   into the *same* table was wrong too — it let case-insensitive matches leak
   into case-sensitive results.)

5. **`HashTable::rehash` computed new bucket indices incorrectly.** It called
   `hashFunction(entry.first) % newHG`, but `hashFunction` already reduces
   modulo the *current* `hashGroups` — so this was `(hash % oldHG) % newHG`,
   not `hash % newHG`. After growth, `hashGroups` is updated to `newHG`, so a
   later `search()`/`insert()` computes the correct `hash % newHG` and looks
   in a different bucket than where `rehash()` actually placed the entry —
   silently losing lookups for existing keys after any table growth. Fixed
   by updating `hashGroups` before recomputing bucket indices.

6. **`@insensitive` had a length bug: `query.substr(0,11)` against the
   12-character word `"@insensitive"`** — an 11-character slice can never
   equal a 12-character literal, so the check was always false and this
   branch was unreachable. Fixed the length to 12 in both the command dispatch
   and `removeCommand`.

7. **Even after fixing (6), `@insensitive` was still unreachable** because
   `processQueries` checked the shorter `"@i"` prefix first, and
   `"@insensitive".substr(0,2) == "@i"` is always true — so the short-form
   branch (and its matching, incorrect strip) always intercepted it first.
   Reordered the checks so the longer, more specific prefix is tested first.

8. **`@f <file>` left a query loop desync.** `cin >> newOutputFile` doesn't
   consume the trailing newline after the filename, so the very next
   `getline(cin, query)` immediately read that leftover empty line as a
   spurious query, producing a stray `" Not Found."` result in the new
   output file. Added `cin.ignore(numeric_limits<streamsize>::max(), '\n')`
   after reading the filename.

### Verification

All fixes were verified together with the actual `DirNode.o`/`FSTree.o`
binaries in a Linux x86-64 container (`gcc:13`, matching the original grading
platform): multi-word lines, repeated words across files and lines,
subdirectory recursion, `@i`/`@insensitive` (short and long form),
case-sensitive vs. case-insensitive isolation, `@f` output redirection, and
not-found queries all now produce correct results.

---

## Native macOS support — building entirely from source

The pass above left one thing unresolved: `DirNode.o` and `FSTree.o` were
precompiled **Linux ELF x86-64** objects with no source anywhere in this
repo's history. Object files are platform-locked, so while the project could
be *relinked* on Linux, it could not be built on macOS at all — a Mach-O
build has nothing it can link those against. That dependency is now gone.

### `DirNode` and `FSTree` reimplemented from source

Both classes were rewritten from their documented interfaces in the existing
`DirNode.h` / `FSTree.h`, which were the one part of that layer that had been
committed. The new `DirNode.cpp` and `FSTree.cpp` implement every declared
method, and `FSTree` walks the real filesystem with POSIX `dirent`/`stat`
(macOS and Linux; see the roadmap for Windows).

Two behaviors were reproduced exactly, having been established empirically
against the old binaries before they were discarded:

- **Node naming.** The root node's name is the path string exactly as passed
  in (`FSTree("some/dir")` → root named `"some/dir"`), while every
  subdirectory node's name is only its own basename (`"nested"`). The rest of
  the program depends on this to rebuild full paths on the way down the tree,
  so getting it backwards would have broken every result path.
- **Ownership.** `DirNode` has no destructor, so nodes do not own their
  children; `FSTree::burnTree` frees the tree post-order, and deep copies go
  through the `preOrderCopy` already defined inline in `FSTree.h`.

Two deliberate improvements over the originals:

- **Directory entries are sorted** before traversal. `readdir` order is
  filesystem-dependent and differs between APFS and ext4, so sorting makes
  gerp's output byte-identical across machines instead of arbitrary.
- **Self-assignment is a no-op rather than a crash.** `DirNode.h` documents
  its assignment operator as "will seg fault if copied onto itself"; the new
  implementation simply guards against it.

### Build

- **Removed `-no-pie`.** It existed solely to link the old non-PIE objects
  and is unnecessary now that everything is compiled from source — it is also
  not something to hand Apple's linker.
- **Fixed the warning flags, which had never been applied.** Every compile
  rule invoked `$(CXX) $(LDFLAGS) -c ...`, so `CXXFLAGS` — and with it
  `-Wall -Wextra -Wpedantic -Wshadow` — was silently unused for the entire
  life of this project. The rules now use `CXXFLAGS`. With the warnings
  genuinely enabled for the first time, the whole codebase compiles clean on
  both `clang++` and `g++`.
- Replaced the per-file rules with a pattern rule plus explicit header
  dependencies, pinned `-std=c++11`, and left `CXX` at make's built-in
  default so the platform's own compiler is used (overridable with
  `make CXX=...`).
- **Deleted `DirNode.o` and `FSTree.o` from the repository** and removed the
  `.gitignore` exceptions that had been keeping them tracked. No binaries
  remain in the repo.

### One more correctness fix (9th bug)

**Result records were being mangled by `stripNonAlphaNum`.** `HashTable::insert`
applied it to the *value* — the fully-formed `path:line: text` record — when
it is only meant to normalize indexed *words* (the caller already applies it
to the key). Because it trims leading and trailing non-alphanumerics, every
absolute path lost its leading `/` (results read `tmp/data/f.txt:1:` instead
of `/tmp/data/f.txt:1:`, so they could not be copied and used), and any
matched line ending in punctuation had that punctuation silently deleted from
the output. The value is now stored verbatim.

### Verification

Built and exercised on **both platforms**, from a clean tree:

- **macOS (Apple clang, arm64)** — builds warning-free, produces a native
  Mach-O arm64 binary.
- **Linux x86-64 (`gcc:13`)** — builds warning-free, no `-no-pie` needed.

Both produce identical, correct results for: case-sensitive and
case-insensitive queries (`@i` and `@insensitive`), words appearing multiple
times within and across files, nested subdirectories (verified to five levels
deep), `@f` output redirection, not-found queries, absolute paths with their
leading `/` intact, and lines ending in punctuation. Edge cases were also
checked — a nonexistent directory, an empty directory, and deeply nested
trees all behave gracefully rather than crashing.
