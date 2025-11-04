## Change log: `src/userland/include/times.h`

- Date: 2025-11-05

### Summary
- Introduced a simple userland time helper declaration.

### Key changes
- Declared `uint32_t time_ms(void);` to expose millisecond time via syscalls.

### Behavior/ABI impact
- Provides a stable function declaration consumed by user code.

### How to test
- Include `times.h` and call `time_ms()`; ensure it links against the wrapper in `unistd.c`.
