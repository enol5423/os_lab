## Change log: `src/kern/kmain/kmain.c`

- Date: 2025-11-05

### Summary
- Added a syscall smoke test at boot and a SysTick demonstration helper to visibly verify the SVC path and timing functions.

### Key changes
- After `__sys_init()` and enabling SysTick, invoke userland wrappers:
  - `write(1, "Hello via SYS_write\r\n", 21);`
  - `kprintf("time_ms() via syscall: %d\r\n", (int)time_ms());`
  - `kprintf("getpid() via syscall: %d\r\n", getpid());`
  - `yield();`
- Added `test_systick_syscalls()` function for extended timing checks (optional to call).
- Removed duplicate demo block to avoid variable redefinition.

### Behavior/ABI impact
- None; purely diagnostic output at boot.

### How to test
- Flash and open serial monitor (115200). Look for the smoke test lines immediately after boot.
