# Operating System Lab Assignment 02 – DUOS Syscalls, SVC, and Context Switching

Version: 1.0 (based on the provided assignment PDF)

This guide summarizes what the assignment asks you to do and gives a practical, step‑by‑step path to complete it in this repository.

---

## What you must build

Implement a basic system call (syscall) mechanism in DUOS using the ARM Cortex‑M SVC (Supervisor Call) exception and use it from userland code. Specifically:

- Provide syscall IDs and a syscall dispatcher in the kernel.
- Implement core services at minimum:
  - SYS_write (printing to the terminal/USART)
  - SYS_read (read from terminal, if available)
  - SYS_exit (terminate current task)
  - SYS_getpid (return current task ID)
  - SYS_time (read system time)
  - SYS_reboot (reset MCU)
  - SYS_yield (voluntary reschedule)
- Wire up userland wrapper functions (e.g., write, read, getpid, time, reboot, exit, yield) that invoke SVC and pass arguments down to the kernel.
- Understand and leverage the exception return (EXC_RETURN) values used by Cortex‑M for returning from exceptions and for context switching.

Optional/next: Task scheduling with SysTick + PendSV and a Round‑Robin policy will be extended in later work, but you should be aware of PSP/MSP usage and callee‑saved register handling now.

---

## Repository map (paths you’ll touch)

- Kernel syscall numbers: `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/kern/include/kern/syscall_def.h`
- Kernel syscall dispatcher & services: `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/kern/syscall/syscall.c`
- Kernel unistd-like constants (STDIN/OUT/ERR): `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/kern/include/kern/kunistd.h`
- Userland syscall wrappers (your current file): `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/userland/utils/unistd.c`
- Build system: `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/compile/Makefile`

Documentation: `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/doc/Readme.txt` and the provided PDF (ass2_pdf).

---

## How SVC-based syscalls flow (quick mental model)

1. Userland calls a wrapper (e.g., `write(fd, buf, n)` in `userland/utils/unistd.c`).
2. The wrapper places arguments as agreed (registers/stack) and executes the `SVC` instruction with the first argument being the syscall ID (from `syscall_def.h`).
3. The CPU enters Handler mode, saves user context to the process stack (PSP), and calls the SVC handler.
4. The kernel SVC handler/dispatcher reads the SVC number and arguments, selects the correct kernel service (e.g., `SYS_write`), runs it (using drivers like USART), and prepares a return value.
5. EXC_RETURN is used to restore context and return to Thread (user) mode via PSP.

---

## EXC_RETURN essentials you need to recognize

Typical values you’ll see on Cortex‑M:
- `0xFFFFFFF9` – Return to Thread mode using MSP
- `0xFFFFFFFD` – Return to Thread mode using PSP (common for RTOS/tasking)
- With FPU variants: `0xFFFFFFE1/E9/ED` include FPU context semantics

Important bits:
- Bit 4: 1→Thread mode, 0→Handler mode
- Bit 3: 1→Use PSP, 0→Use MSP
- Bit 2: 1→No FP context, 0→FP context stacked

Your SVC and PendSV paths should set LR to the appropriate EXC_RETURN so you return correctly to user mode using PSP.

---

## Step‑by‑step plan to finish the assignment

Follow these steps in order. Items with [Check] are the expected outcomes.

1) Read and align on IDs and contracts
- Open `syscall_def.h` and identify the numeric IDs for services you’ll implement (`SYS_write`, `SYS_read`, `SYS_exit`, `SYS_getpid`, `SYS_time`, `SYS_reboot`, `SYS_yield`).
- Define a simple calling convention (inputs/outputs) and error returns for each service.
  - Example contract for write:
    - Inputs: r0 = SYS_write, r1 = fd, r2 = ptr, r3 = size
    - Output: r0 = bytes written (>=0) or negative errno
- [Check] A small comment block in `syscall.c` documenting register usage.

2) Implement userland wrappers (`userland/utils/unistd.c`)
- For each function (write, read, exit, getpid, time, reboot, yield):
  - Marshal arguments into r0–r3 (as per your contract)
  - Issue `SVC #0` (the actual SVC immediate can be 0; you’ll read the SVC number from stacked PC in the handler or pass the number in r0 — stick to one convention consistently.)
  - Return kernel result to the caller.
- [Check] Minimal smoke test: calling `write(STDOUT_FILENO, "hi", 2)` reaches the kernel and prints via USART.

3) Kernel SVC handler and dispatcher (`kern/syscall/syscall.c`)
- In the SVC handler, extract the syscall ID and arguments.
  - Common pattern: read stacked PC, fetch the SVC immediate, or read r0 if you pass ID in a register. The assignment description allows either approach; be consistent with step 2.
- Use a switch on syscall ID to call the right kernel function.
- Return values in r0; ensure the exception unwinds with the correct EXC_RETURN (likely `0xFFFFFFFD` to return to Thread mode using PSP).
- [Check] Each required syscall returns a sensible value and does not corrupt context.

4) Implement the kernel services
- `SYS_write`: format/send bytes through the USART driver already present under `src/kern/lib` or `src/kern/dev` (see `sys_usart.c` and friends under `stm32f446re/sys_lib`).
- `SYS_read`: optionally read from input (if your lab expects it; otherwise stub with -ENOSYS or 0 for nonblocking console).
- `SYS_exit`: mark current TCB as TERMINATED and trigger a reschedule (or return a code if full process teardown is not ready yet).
- `SYS_getpid`: return `current_tcb->task_id`.
- `SYS_time`: return a millisecond tick from SysTick or your time service.
- `SYS_reboot`: call the platform reset logic.
- `SYS_yield`: pend PendSV (trigger context switch) and return 0.
- [Check] No hard faults during repeated calls; USART output visible.

5) Validate context switching assumptions
- Ensure your build uses PSP for user tasks and MSP for exceptions.
- In your PendSV handler (if present), save/restore r4–r11, update TCB PSP, and restore next task’s PSP before `BX lr`.
- [Check] After `SYS_yield`, a different task runs (if you have >1 task); otherwise, flow returns cleanly to the same task.

6) Testing and demos
- Add a tiny user task that:
  - Prints a line with `write`.
  - Shows `getpid` value.
  - Prints `time()` before/after a busy wait.
  - Calls `yield()` a few times.
- Optional: a second task to show round‑robin effects.
- [Check] All calls behave per your contracts; no lockups.

7) Submission artefacts to prepare
- Code changes in `syscall.c`, wrappers in `unistd.c`, and any small test code you added.
- A short note or README snippet explaining the calling convention and which syscalls are fully implemented.

---

## Minimal syscall checklist (copy into your notes)

- [ ] SYS_write via USART (fd = STDOUT_FILENO)
- [ ] SYS_read (stub or real)
- [ ] SYS_exit
- [ ] SYS_getpid
- [ ] SYS_time (ms tick)
- [ ] SYS_reboot
- [ ] SYS_yield
- [ ] SVC handler extracts ID and dispatches correctly
- [ ] EXC_RETURN returns to Thread mode with PSP

---

## Build and run tips (Windows)

- The build system is Makefile‑based at `src/compile/Makefile`.
- On Windows, you can:
  - Use WSL (Ubuntu) and build inside WSL for a smoother GCC/Make toolchain.
  - Or use MSYS2/MinGW or Git Bash if your toolchain supports the target (ARM GCC).
- Typical flow:
  - Open a shell in `Lab_1_an/Lab_1/CSE-3211__DUOS_labs/src/compile/`
  - Run the build (e.g., `make`) to produce the firmware in `target/duos`
  - Flash to your STM32 board using your usual tool (ST‑Link, OpenOCD, etc.)

Note: exact commands depend on your local toolchain; adapt as needed.

---

## Design notes and gotchas

- Keep a clear, single calling convention for passing syscall ID and arguments. Document it near the dispatcher.
- Save/restore rules: exceptions auto‑stack r0–r3, r12, LR, PC, xPSR; PendSV typically handles r4–r11. Don’t double‑stack.
- If a service isn’t implemented yet, return `-ENOSYS` to avoid crashes.
- Be mindful of privileges: user code runs unprivileged; kernel runs privileged. SVC transitions privilege for you.
- Validate pointers from userland before dereferencing in the kernel to avoid faults.

---

## Appendix: Useful references inside this repo

- `sys_usart.c` and related HAL under: `src/kern/arch/stm32f446re/sys_lib/`
- Headers for platform specifics: `src/kern/arch/stm32f446re/include/`
- Error and utilities: `src/kern/lib/` and `src/kern/include/kern/`

---

## What the PDF emphasized (condensed)

- Explore SVC and how it saves context and returns using EXC_RETURN.
- Learn how the syscall number is determined and how the handler dispatches.
- Understand how user arguments flow to kernel services and results come back.
- Gain hands‑on experience with user vs. kernel mode and protected operations.

Alert from the doc: you may discuss with classmates, but do not copy code.

---

If you want, I can scaffold the `unistd.c` wrappers and a basic dispatcher switch in `syscall.c` next so you have a working baseline to iterate on.