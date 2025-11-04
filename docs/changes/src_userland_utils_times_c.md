## Change log: `src/userland/utils/times.c`

- Date: 2025-11-05

### Summary
- Left intentionally minimal to avoid duplicate symbol with `time_ms()` provided in `unistd.c` wrapper layer.

### Key changes
- Documented that `time_ms()` implementation is centralized in `unistd.c` and this file remains as a placeholder.

### Behavior/ABI impact
- No functional code here; ensures single definition of `time_ms` across userland.

### How to test
- Build the project; verify no duplicate symbol errors for `time_ms` and calls succeed.
