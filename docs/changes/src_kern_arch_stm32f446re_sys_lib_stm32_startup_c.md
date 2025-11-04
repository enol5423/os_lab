## Change log: `src/kern/arch/stm32f446re/sys_lib/stm32_startup.c`

- Date: 2025-11-05

### Summary
- Wired up a real `SVCall_Handler` that extracts the stacked arguments, calls the C dispatcher, and writes back the return value.

### Key changes
- Vector table already points to `SVCall_Handler`; ensured handler is implemented.
- In `SVCall_Handler`:
  - Detects stack pointer (MSP/PSP) per exception return.
  - Reads stacked r0–r3 (syscall id + args).
  - Calls `syscall_dispatch(callno, a1, a2, a3)`.
  - Writes return value to stacked r0 so the user context sees it after `svc`.

### Behavior/ABI impact
- Establishes the SVC gateway into the kernel as the only entry for userland syscalls.

### How to test
- Use `write()`/`time_ms()` user wrappers and confirm correct values are returned/printed.

### Notes
- Fault handlers remain as-is; no changes to other vectors.
