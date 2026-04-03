## Build Commands
- `cmake --build builds/Debug --parallel` - Build the project
- `./builds/Debug/src/octopus/test/unit_tests` - Run all tests

## Code Style
- Infer code style from existing code
- **No exceptions**: the codebase must build with `-fno-exceptions`. Use `assert()` for programmer errors instead of `throw`.
- **No floating-point**: do not use `float`, `double`, or any floating-point literals/operations anywhere in the codebase. All arithmetic must use integer types (e.g. `long long`, `int64_t`) or `octopus::Fixed` to guarantee deterministic results across different hardware.

## Workflow
- Always run tests before tyding up code
- Commit messages follow conventional commits format
- Create feature branches from `main`
