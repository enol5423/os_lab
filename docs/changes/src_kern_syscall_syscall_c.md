## Change log: `src/kern/syscall/syscall.c`

- Date: 2025-11-05

### Summary
- Implemented a kernel syscall dispatcher with core services and minimal task hook support.

### Key changes
- Added `int32_t syscall_dispatch(uint16_t callno, uint32_t a1, uint32_t a2, uint32_t a3)`.
- Implemented handlers:
  - `SYS_write` → `putstr` to console; returns bytes written for fd=1/2, else -ENOSYS.
  - `SYS___time` → returns `__getTime()` milliseconds.
  - `SYS_reboot` → `NVIC_SystemReset()`.
  - `SYS_yield` → sets `SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk` and returns 0.
  - `SYS_getpid` → returns 1 (placeholder until TCBs exist).
  - `SYS__exit` → enters low-power loop via `__WFI()`.
  - `SYS_read` → returns -ENOSYS (stub).

### Behavior/ABI impact
- Defines return convention (non-negative success; negative errno on failure like -ENOSYS/-EINVAL).
- Yield does not perform a real context switch yet; it safely returns.

### How to test
- Trigger via userland wrappers (`write`, `time_ms`, `getpid`, `yield`, `_exit`).
- Observe serial output for `write` and monotonic increase for `time_ms`.

### Notes
- Extend `SYS_yield` + `PendSV_Handler` later with TCB save/restore and PSP swap for round-robin.
