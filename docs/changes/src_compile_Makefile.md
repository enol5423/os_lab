## Change log: `src/compile/Makefile`

- Date: 2025-11-05

### Summary
- Verified build targets include newly added/used objects for the syscall path; no changes were necessary.

### Observations
- Objects present: `unistd.o`, `times.o`, `syscall.o`, `cm4.o`, `stm32_startup.o`, `kmain.o`, etc.
- Link step produces `target/duos` and `build/final.map` as configured.

### Behavior/ABI impact
- None.

### How to test
- Run build from `src/compile/` and confirm successful compilation and linking.
