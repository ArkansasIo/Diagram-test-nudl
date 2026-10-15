# Architecture

## Node model

Each node has a stable identifier, kind, optional literal or variable reference, operator, and child nodes. Children are shared pointers for convenient graph construction; validation currently rejects duplicate identifiers but assumes callers construct a tree rather than a general directed graph.

Node kinds reserved for extension include modules, classes, types, and arrays. In this starter runtime, module/class/type nodes behave as ordered containers and array nodes evaluate their children in order. They do not yet implement module imports, object instantiation, static type checking, or array-valued runtime values.

## Evaluation contract

- Every node returns either a successful typed `Value` or a failure with node ID and message.
- A Boolean condition must return an actual Boolean value; no truthy/falsy coercion is performed.
- Branch children are `[condition, thenBody, optionalElseBody]`.
- Sequence-like nodes evaluate children left-to-right and return the last successful child value.
- AND/OR short-circuit; NOT requires one Boolean operand.
- Arithmetic operations require numeric operands. Division by zero fails.
- Undefined variable references fail.
- Depth, node-count, and trace limits constrain the runtime.

## Security and safety

The current runtime does not evaluate source text and does not expose filesystem or network operations. A future function/module registry should use an allowlist and explicit capabilities. For untrusted programs, add memory and instruction budgets as well as cancellation.

## Planned expansion

1. Lexer, grammar, and parser for NUDL source.
2. Symbol scopes, type registry, type inference/checking, and generic constraints.
3. Array-valued `Value` support and collection operations.
4. Modules, imports, functions, classes, instance state, and inheritance.
5. Data-flow/dependency graph and scheduler.
6. Transaction boundaries, rollback hooks, serialization schema/versioning, and cache dependency tracking.
7. Fuzz testing, property tests, benchmarks, and structured diagnostics.
