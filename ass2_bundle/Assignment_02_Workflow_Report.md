# Assignment 02 – My Implementation Workflow (Syscalls + Minimal Task Hook)

This is my own step-by-step log of what I did to implement system calls (via SVC) and a minimal task-management hook (PendSV yield) in the DUOS codebase, plus how I tested it on the STM32F446RE.

I’m writing it in my own words to explain the choices I made, the files I touched, and how to verify the result.

---

## Quick overview
- Goal: Implement a working SVC-based syscall mechanism and a tiny yield hook using PendSV, with visible serial output to prove it works.
- Platform: STM32F446RE (NUCLEO board), toolchain `arm-none-eabi-gcc`, flashing with OpenOCD, serial via ST‑LINK VCP at 115200.

---

## What I implemented
1) A real SVC handler that reads the stacked registers and calls a C dispatcher.
2) A syscall dispatcher in the kernel that implements core services:
   - SYS_write → print to console (USART)
   - SYS___time → return SysTick milliseconds
   - SYS_reboot → reset MCU
   - SYS_yield → set PendSV pending bit
   - SYS_getpid → return a placeholder PID (1 for now)
   - SYS__exit → stop the task in a low-power loop
   - SYS_read → currently returns -ENOSYS (not implemented)
3) Userland wrapper functions that issue `svc 0` with r0=call number and r1–r3 as args.
4) A minimal `PendSV_Handler` so `yield()` can safely pend and return (starter for a real scheduler later).
5) A small syscall smoke test at boot (in `kmain`) so I can see it working in the serial monitor immediately after flashing.

---

## Files I edited and why
- Kernel (SVC and dispatch)
  - `src/kern/arch/stm32f446re/sys_lib/stm32_startup.c`
    - Implemented `SVCall_Handler` to:
      - Detect which stack pointer (MSP/PSP) was used on exception entry
      - Read stacked r0–r3 (syscall id and args)
      - Call `syscall_dispatch(callno, a1, a2, a3)`
      - Put the return value back into stacked r0
  - `src/kern/syscall/syscall.c`
    - Implemented `int32_t syscall_dispatch(uint16_t, uint32_t, uint32_t, uint32_t)` with the switch for the syscalls listed above.
    - Used `putstr(...)` for writes and `__getTime()` for time. For yield, I set `SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk`.
  - `src/kern/include/syscall.h`
    - Added the dispatcher prototype and kept the original prototype for safety.
  - `src/kern/arch/cm4/cm4.c`
    - Added a minimal `PendSV_Handler()` that simply clears the pending bit (so `yield()` won’t hard fault). This is the place to extend later with real context switching.

- Userland (wrappers and headers)
  - `src/userland/include/unistd.h`
    - Declared `write/read/_exit/getpid/time_ms/reboot/yield` and used project’s `types.h` to avoid size_t conflicts.
  - `src/userland/utils/unistd.c`
    - Implemented the wrappers using inline `svc 0`. Contract: r0=callno; r1–r3=arguments; return in r0.
  - `src/userland/include/times.h`
    - Declared `time_ms()`.
  - `src/userland/utils/times.c`
    - Left minimal (to avoid duplicate symbol). `time_ms()` is provided via `unistd.c`.

- Boot and demo
  - `src/kern/kmain/kmain.c`
    - After `__sys_init()` and enabling SysTick, I added a short “syscall smoke test”: `write(...)`, `time_ms()`, `getpid()`, `yield()` with `kprintf` lines to show results.

- Docs
  - `Assignment_02_Guide.md` – my plan and scope
  - `Assignment_02_Implementation.md` – what changed and how to test
  - This file – my personal workflow report

---

## The workflow I followed (step by step)
1) Align on syscall IDs and calling convention
   - I checked `src/kern/include/kern/syscall_def.h` for IDs.
   - I chose the simple convention: r0 = syscall id; r1–r3 = up to 3 args; return value in r0.

2) Wire the SVC handler
   - Implemented `SVCall_Handler` in the startup file to extract stacked registers (from MSP or PSP), call my C dispatcher, and write back the return value to stacked r0.

3) Implement the kernel dispatcher
   - Added `syscall_dispatch(...)` in `syscall.c` with a `switch(callno)` and handlers for the main syscalls.
   - I used existing console utils (`putstr`, USART) and time function (`__getTime`).
   - For `yield`, I set the PendSV pending bit; for `reboot`, I called `NVIC_SystemReset()`.

4) Add userland wrappers
   - In `unistd.c`, I wrote a small `svc_invoke()` helper and routed `write/read/getpid/...` through it.
   - I fixed a `size_t` conflict by including the project’s `types.h` instead of redefining `size_t` on my own.

5) Minimal PendSV
   - Wrote a stub `PendSV_Handler` so `yield()` can work safely without implementing the full context switch yet. This is where I’d save/restore r4–r11 and swap PSP in the future.

6) Boot-time smoke test
   - In `kmain.c`, right after enabling SysTick, I added:
     - `write(1, "Hello via SYS_write\r\n", 21);`
     - `kprintf("time_ms() via syscall: %d\r\n", (int)time_ms());`
     - `kprintf("getpid() via syscall: %d\r\n", getpid());`
     - `kprintf("yield() returned: %d\r\n", yield());`
   - This gives immediate visible proof in the serial log.

7) Build, flash, and test
   - Build in `src/compile` with `make all`.
   - Flash using OpenOCD (from the same folder):
     ```powershell
     openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program target/duos verify reset exit"
     ```
   - Open VS Code Serial Monitor (ST‑LINK COM port) at 115200 8‑N‑1, then press RESET.
   - I looked for the smoke test lines and the SysTick demo prints.

---

## How I verified it works
- I saw the lines from the smoke test:
  - Hello via SYS_write
  - time_ms() via syscall: <number>
  - getpid() via syscall: 1
  - yield() returned: 0
- The rest of the SysTick demo also runs (shows time and delays), which tells me the board is healthy and the system clock and SysTick are configured.
- Optional stronger check I considered: add a `pendsv_count` variable incremented inside `PendSV_Handler` and print its value after `yield()`. (Easy to add later to prove the handler fired.)

---

## Gotchas I handled
- size_t conflict:
  - The project defines `size_t` in `kern/include/kern/types.h` (as a macro). I removed my redefinition in `userland/include/unistd.h` and included the project header instead. That fixed the compiler error.
- Linking the dispatcher:
  - Ensured `syscall.c` builds to `object/syscall.o` and gets linked into the final ELF so `SVCall_Handler` can resolve `syscall_dispatch`.
- Serial output setup:
  - The console points to `USART2` (ST‑LINK VCP) at 115200. I used VS Code’s Serial Monitor and pressed RESET after opening it.

---

## What’s left / next steps
- Make `getpid()` read from a real TCB when I add task structures.
- Implement a Round‑Robin scheduler in `PendSV_Handler`:
  - Save `r4–r11`, store PSP to current TCB, load next TCB’s PSP, restore `r4–r11`, return with `EXC_RETURN_THREAD_PSP`.
- Implement `read()` using the UART ring buffer so I can read from the terminal.

---

## Why I believe this is correct
- The SVC path is exercised by userland wrappers and produces correct results on the serial console.
- The handler reads stacked registers and returns values via stacked r0, which matches the Cortex‑M exception return convention.
- No hard faults: yield sets PendSV and returns safely with my stub handler.

---

## Try it yourself
- Build:
  ```powershell
  make all
  ```
- Flash:
  ```powershell
  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program target/duos verify reset exit"
  ```
- Serial monitor: open ST‑LINK COM port at 115200 8‑N‑1 and press RESET.

If you want me to, I can add the optional PendSV counter to visibly confirm that `yield()` fired the handler.
