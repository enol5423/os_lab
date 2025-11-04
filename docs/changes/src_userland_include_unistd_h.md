## Change log: `src/userland/include/unistd.h`

- Date: 2025-11-05

### Summary
- Declared syscall-like user APIs and aligned types with the kernel to avoid conflicts.

### Key changes
- Included `<types.h>` to use the project-defined `size_t` and integer types.
- Defined `ssize_t` and `pid_t` locally.
- Declared wrappers: `write`, `read`, `_exit`, `getpid`, `time_ms`, `reboot`, `yield`.

### Behavior/ABI impact
- Avoids prior `size_t` redefinition conflicts by relying on project types.

### How to test
- Include this header in user code and call the wrappers; should compile without type conflicts.
