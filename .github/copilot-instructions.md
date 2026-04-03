## Build Commands
- `cmake --build builds/Debug --parallel` - Build the project
- `./builds/Debug/src/octopus/test/unit_tests` - Run all tests

## Code Style
- Infer code style from existing code
- **No exceptions**: the codebase must build with `-fno-exceptions`. Use `assert()` for programmer errors instead of `throw`.

## Workflow
- Always run tests before tyding up code
- Commit messages follow conventional commits format
- Create feature branches from `main`
