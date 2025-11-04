# Assignment 02 — Syscall and Task Management: What I implemented, how, and how to test

This document explains the concrete changes made to implement the syscall mechanism (via SVC) and a minimal task-management hook (PendSV yield) in your DUOS tree.

---

## Summary of changes

- Implemented an SVC-based syscall path end-to-end
  - Userland wrappers (`write`, `read` stub, `getpid`, `time_ms`, `reboot`, `yield`, `_exit`) that trap into kernel via `svc 0`.
  - A real SVC handler that extracts the stacked context, calls a C dispatcher, and writes the return value back to stacked `r0`.
  - A kernel syscall dispatcher with working handlers for:
    - `SYS_write` → prints via USART
    - `SYS___time` → returns SysTick milliseconds
    - `SYS_reboot` → triggers MCU reset
    - `SYS_yield` → pends a PendSV interrupt (cooperative yield)
    - `SYS_getpid` → returns a placeholder pid (1)
    - `SYS__exit` → halts the task (low power loop)
- Added a minimal `PendSV_Handler` so `yield()` does not hard fault and can be extended later to perform context switching.

---

## Files touched (key points)

- Kernel (SVC and dispatch):
  - `src/kern/arch/stm32f446re/sys_lib/stm32_startup.c`
    - Implemented `SVCall_Handler`: reads stacked `r0..r3`, calls `syscall_dispatch()`, writes return value to stacked `r0`.
  - `src/kern/syscall/syscall.c`
    - Implemented `syscall_dispatch(...)` switch for core syscalls.
    - Uses `putstr(...)` for writes, `__getTime()` for time, `NVIC_SystemReset()` for reboot, and sets `PENDSVSET` for yield.
  - `src/kern/include/syscall.h`
    - Added prototype `int32_t syscall_dispatch(uint16_t,uint32_t,uint32_t,uint32_t);` (kept legacy `void syscall(uint16_t)` to avoid breakage).
  - `src/kern/arch/cm4/cm4.c`
    - Added a minimal `PendSV_Handler()` that simply clears the pending flag (placeholder for future context switch code).
- Userland (wrappers):
  - `src/userland/include/unistd.h`
    - Added prototypes and small typedefs for `size_t`, `ssize_t`, `pid_t`.
  - `src/userland/utils/unistd.c`
    - Implemented wrappers using inline `svc 0` with `r0`=syscall id, `r1..r3`=args.
  - `src/userland/include/times.h` and `src/userland/utils/times.c`
    - Added `time_ms()` helper that directly SVCs `SYS___time`.

---

## Calling convention and contract

- Inputs (on SVC entry):
  - `r0` = syscall number (`syscall_def.h`)
  - `r1..r3` = up to three arguments
- Return:
  - `r0` = return value (>=0 on success, negative errno on failure such as `-ENOSYS`)
- Implemented syscalls:
  - `SYS_write(fd, ptr, size)` → returns bytes written; only `STDOUT_FILENO` and `STDERR_FILENO` are supported.
  - `SYS_read(...)` → returns `-ENOSYS` (stub).
  - `SYS___time()` → returns milliseconds since boot (`__getTime()`).
  - `SYS_reboot()` → resets MCU.
  - `SYS_yield()` → sets `PENDSVSET` (see task section below).
  - `SYS_getpid()` → returns 1 (placeholder).
  - `SYS__exit(code)` → halts the current thread of execution.

---

## Minimal task-management hook (PendSV yield)

- `yield()` in userland triggers `SYS_yield`.
- Kernel `SYS_yield` sets `SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk` to request a PendSV.
- `PendSV_Handler()` currently just clears the pending bit and returns. This avoids a hard fault and forms the skeleton you can extend with a full context switcher:
  - Save `r4..r11`
  - Save current PSP to the current TCB
  - Load next TCB’s PSP and restore `r4..r11`
  - Exit with `EXC_RETURN_THREAD_PSP`

This covers the “task management” requirement at a minimal, safe level. You can iterate from here to implement round-robin.

---

## How to test

1) Build
- From `src/compile/`, build the firmware with your usual toolchain (Makefile is already provided). The output binary appears under `src/compile/target/duos`.

2) Basic smoke test via `kmain()`
- On boot you should see console output from `__sys_init()` and the time demos already present.

3) Userland syscall tests
- Add quick calls in your user task (or wherever you run userland code):
  - `write(1, "Hello via SYS_write\n", 22);`
  - `uint32_t t = time_ms(); write(1, "t=", 2);` … print `t` using existing `kprintf` or convert utility.
  - `getpid();`
  - `yield();` (no visible change yet, but should not hard fault)
  - Optionally try `reboot();` to verify reset.

4) Expected results
- `SYS_write` prints to the USART console when `fd` is 1 or 2.
- `time_ms()` returns an increasing millisecond counter.
- `yield()` returns 0 and system continues to run.

5) Error paths
- Unsupported syscalls return `-ENOSYS`.
- `SYS_write` on unsupported `fd` returns `-ENOSYS`.

---

## Notes and next steps

- The current `PendSV_Handler` is a placeholder. To complete round‑robin scheduling, add a TCB list/queue and implement save/restore of callee-saved registers and PSP switching.
- `SYS_read` is left as a stub; you can connect it to the UART ring buffer (`UsartRingBuffer`) for terminal input.
- If you introduce a TCB, you can replace `getpid()` with `current_tcb->task_id`, and make `_exit()` mark the task TERMINATED and pend a reschedule.

---

## Pointers in the repo

- Syscall IDs: `src/kern/include/kern/syscall_def.h`
- Dispatcher: `src/kern/syscall/syscall.c`
- User wrappers: `src/userland/utils/unistd.c`
- SVC handler: `src/kern/arch/stm32f446re/sys_lib/stm32_startup.c`
- USART and kprintf: `src/kern/lib/kstdio.c`, `src/kern/arch/stm32f446re/include/sys_usart.h`

If you want, I can add a tiny demo task that exercises the wrappers automatically on boot and prints the results.
