## Change log: `src/kern/lib/kern/sys_init.c`

- Date: 2025-11-05

### Summary
- Augmented boot logs and added a `display_group_info()` helper for clear demo output.

### Key changes
- Printed board/OS banner and member info.
- Called `display_group_info()` during init to show SysTick values and a delay test.

### Behavior/ABI impact
- No kernel behavior change; improved observability via serial output.

### How to test
- Boot and review the printed banner and timing diagnostics on the serial console.
