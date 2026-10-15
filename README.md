# Diagram Test NUDL — Master Logic Tree Algorithms

A C++23 starter implementation and specification for a recursive master-logic engine. The engine models statements as a tree of nodes and evaluates sequences, branches, conditions, typed values, and nested arrays.

## Status

This repository was empty when initialized. This first commit establishes the project skeleton and a working, deliberately small runtime core. It is not yet a complete NUDL compiler or production interpreter.

## Architecture

- **Frontend:** source text, tokens, parser, AST (planned extension points).
- **Logic tree:** root statement, child statements, branches, modules, classes, type declarations, and arrays.
- **Runtime:** typed values, recursive evaluation, Boolean conditions, execution results, errors, and trace.
- **Tooling:** CMake, C++23, unit tests, GitHub Actions CI, and NUDL-style example.

See [docs/ALGORITHMS.md](docs/ALGORITHMS.md) for the algorithm suite and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for design notes.

## Build and test

Requires a C++23 compiler and CMake 3.20 or newer.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the example:

```sh
./build/mlta_demo
```

## Scope and semantics

The current runtime uses a typed value variant containing null, Boolean, integer, floating-point, and string values. It supports sequential nodes, explicit Boolean conditions, branch selection, and array-element traversal. Parser, user-defined type registry, class inheritance, module loading, transactions, parallel scheduling, serialization, and optimizations are documented extension points and are not claimed as implemented.

## License

No license has been selected for this repository yet. Add a license before redistributing the project.
