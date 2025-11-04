## Change log: `src/userland/utils/unistd.c`

- Date: 2025-11-05

### Summary
- Implemented userland syscall wrappers and a generic `svc 0` invoker to enter the kernel SVC path.

### Key changes
- Added `svc_invoke(callno, a1, a2, a3)` that marshals arguments into r0–r3 and executes `svc 0`.
- Implemented wrappers:
  - `ssize_t write(int fd, const void *buf, size_t count)`
  - `ssize_t read(int fd, void *buf, size_t count)` (kernel currently returns -ENOSYS)
  - `void _exit(int status)` (traps then loops)
  - `pid_t getpid(void)`
  - `uint32_t time_ms(void)`
  - `int reboot(void)`
  - `int yield(void)`

### Behavior/ABI impact
- Establishes the calling convention: r0 = syscall ID, r1–r3 = up to three args, return in r0.
- No libc dependency; thin wrappers directly trap to kernel.

### How to test
- From `kmain`, call:
  - `write(1, "Hello via SYS_write\r\n", 21);`
  - `kprintf("time_ms() via syscall: %d\r\n", (int)time_ms());`
  - `kprintf("getpid() via syscall: %d\r\n", getpid());`
  - `yield();` (should return 0; system continues)

### Notes
- `read` will return -ENOSYS until the kernel implements it.
