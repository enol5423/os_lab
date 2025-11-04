## Change log: `src/kern/arch/cm4/cm4.c`

- Date: 2025-11-05

### Summary
- Added a minimal `PendSV_Handler` to safely acknowledge `yield()` calls without a full scheduler.

### Key changes
- Implemented `PendSV_Handler()` stub that clears the pending bit and returns.
- Left existing SysTick/time utilities unchanged.

### Behavior/ABI impact
- `SYS_yield` will pend a PendSV and return 0; no context switch is performed yet.

### How to test
- Call `yield()` from userland and check that the system continues to run (no hard fault) and returns 0.

### Notes
- This is the placeholder where callee-saved registers (r4–r11) and PSP swap will be added for round-robin.
