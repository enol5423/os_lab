## Change log: `src/kern/include/syscall.h`

- Date: 2025-11-05

### Summary
- Extended the syscall header with a C dispatcher prototype while keeping the legacy symbol for compatibility.

### Key changes
- Kept `void syscall(uint16_t);` (legacy, unused in new path).
- Added `int32_t syscall_dispatch(uint16_t callno, uint32_t a1, uint32_t a2, uint32_t a3);` used by `SVCall_Handler`.

### Behavior/ABI impact
- Allows the startup SVC handler to call into C for syscall processing.

### How to test
- Build; ensure no undefined-reference errors for `syscall_dispatch` and syscalls function at runtime.
