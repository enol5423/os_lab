# DUOS System Call Implementation Workflow

## Assignment 02: Design and Deploy Syscall
**Course:** CSE-3211 Operating System Lab  
**University of Dhaka, Department of Computer Science and Engineering**

---

## Table of Contents
1. [Overview](#overview)
2. [SVC Exception Mechanism](#svc-exception-mechanism)
3. [Syscall Architecture](#syscall-architecture)
4. [Implemented Syscalls](#implemented-syscalls)
   - [SYS_write](#1-sys_write)
   - [SYS_read](#2-sys_read)
   - [SYS___time](#3-sys___time)
   - [SYS_getpid](#4-sys_getpid)
   - [SYS_yield](#5-sys_yield)
   - [SYS_reboot](#6-sys_reboot)
   - [SYS__exit](#7-sys__exit)
5. [File Structure](#file-structure)
6. [Testing](#testing)

---

## Overview

System calls (syscalls) provide a controlled interface for unprivileged user applications to request services from the privileged kernel. In ARM Cortex-M4, the **SVC (Supervisor Call)** instruction triggers an exception that transitions the processor from unprivileged Thread mode to privileged Handler mode.

### Key Concepts
- **Unprivileged Mode:** User applications run here with restricted access to hardware
- **Privileged Mode:** Kernel runs here with full access to system resources
- **SVC Instruction:** `SVC #imm` triggers SVCall exception for kernel service requests
- **Context Switching:** CPU automatically saves/restores registers during exception entry/exit

---

## SVC Exception Mechanism

### Exception Entry Flow

```
User Application (Unprivileged)
        │
        ▼
   SVC #0 instruction
        │
        ▼
┌───────────────────────────────────┐
│   Hardware Auto-Stacks Registers  │
│   R0, R1, R2, R3, R12, LR, PC, xPSR │
│   onto Process Stack (PSP)        │
└───────────────────────────────────┘
        │
        ▼
   SVCall_Handler (Assembly)
        │
        ▼
   SVCall_Handler_C (C code)
        │
        ▼
   syscall_dispatch()
        │
        ▼
   Specific syscall implementation
        │
        ▼
   Return value → stacked R0
        │
        ▼
   Exception Return (EXC_RETURN)
        │
        ▼
User Application Resumes (with result in R0)
```

### SVCall_Handler Implementation

**File:** `src/kern/arch/stm32f446re/sys_lib/stm32_startup.c`

```c
__attribute__((naked)) void SVCall_Handler(void)
{
    __asm volatile (
        "tst   lr, #4        \n"  // Test bit 2 of EXC_RETURN
        "ite   eq            \n"  // If-Then-Else
        "mrseq r0, msp       \n"  // If 0: r0 = MSP (Main Stack)
        "mrsne r0, psp       \n"  // If 1: r0 = PSP (Process Stack)
        "b     SVCall_Handler_C \n"
    );
}
```

**Explanation:**
- `LR` contains `EXC_RETURN` value after exception entry
- Bit 2 of `EXC_RETURN` indicates which stack was used:
  - `0` → Main Stack Pointer (MSP) - used by handlers
  - `1` → Process Stack Pointer (PSP) - used by threads
- The stack pointer is passed to C handler to access stacked arguments

### SVCall_Handler_C Implementation

```c
void SVCall_Handler_C(uint32_t *stack_ptr)
{
    // Extract syscall number and arguments from stacked frame
    uint16_t callno = (uint16_t)(stack_ptr[0]);  // R0 = syscall ID
    uint32_t a1 = stack_ptr[1];                   // R1 = arg1
    uint32_t a2 = stack_ptr[2];                   // R2 = arg2
    uint32_t a3 = stack_ptr[3];                   // R3 = arg3

    // Dispatch to appropriate handler
    int32_t ret = syscall_dispatch(callno, a1, a2, a3);

    // Place return value in stacked R0
    stack_ptr[0] = (uint32_t)ret;
}
```

### Stack Frame Layout

```
Higher Address
┌─────────────┐
│    xPSR     │  stack_ptr[7]
├─────────────┤
│     PC      │  stack_ptr[6]
├─────────────┤
│     LR      │  stack_ptr[5]
├─────────────┤
│    R12      │  stack_ptr[4]
├─────────────┤
│     R3      │  stack_ptr[3]  ← arg3
├─────────────┤
│     R2      │  stack_ptr[2]  ← arg2
├─────────────┤
│     R1      │  stack_ptr[1]  ← arg1
├─────────────┤
│     R0      │  stack_ptr[0]  ← syscall_id / return value
└─────────────┘
Lower Address (stack_ptr)
```

---

## Syscall Architecture

### Three-File Structure (per Assignment)

| File | Purpose |
|------|---------|
| `syscall_def.h` | Defines unique syscall numbers |
| `syscall.h` | Function prototypes for kernel services |
| `syscall.c` | Implementation of syscall handlers |

### Userland Wrapper Pattern

**File:** `src/userland/utils/unistd.c`

```c
static inline int svc_invoke(uint32_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
    register uint32_t r0 __asm__("r0") = callno;
    register uint32_t r1 __asm__("r1") = a1;
    register uint32_t r2 __asm__("r2") = a2;
    register uint32_t r3 __asm__("r3") = a3;
    __asm volatile ("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return (int)r0;
}
```

---

## Implemented Syscalls

### 1. SYS_write

**Syscall Number:** `55` (defined in `syscall_def.h`)

**Purpose:** Write bytes to a file descriptor (stdout/stderr for terminal output)

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  write(fd, buf, count)                                      │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS_write, fd, buf, count)                      │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(55, fd, buf, count)                       │
│       │                                                     │
│       ▼                                                     │
│  sys_write_impl(fd, buf, len)                               │
│       │                                                     │
│       ├─► if fd == STDOUT_FILENO (1) or STDERR_FILENO (2)   │
│       │       │                                             │
│       │       ▼                                             │
│       │   for each byte in buffer:                          │
│       │       noIntWrite(&huart2, byte)  // Polling UART TX │
│       │       │                                             │
│       │       ▼                                             │
│       │   return bytes_written                              │
│       │                                                     │
│       └─► else: return -ENOSYS (not supported)              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_write_impl(int fd, const uint8_t *buf, uint32_t len)
{
    if (buf == 0U) return -EINVAL;
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        extern UART_HandleTypeDef huart2;
        for (uint32_t i = 0; i < len; i++) {
            noIntWrite(&huart2, (char)buf[i]);  // Polling-based TX
        }
        return (int32_t)len;
    }
    return -ENOSYS;
}
```

**Why `noIntWrite()` instead of `Uart_write()`?**
- SVC handler runs in exception context
- Interrupt-driven TX would deadlock (lower-priority interrupts masked)
- Polling-based write directly checks TXE flag and writes to DR register

---

### 2. SYS_read

**Syscall Number:** `50` (defined in `syscall_def.h`)

**Purpose:** Read bytes from a file descriptor (stdin for terminal input)

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  read(fd, buf, count)                                       │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS_read, fd, buf, count)                       │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(50, fd, buf, count)                       │
│       │                                                     │
│       ▼                                                     │
│  sys_read_impl(fd, buf, len)                                │
│       │                                                     │
│       ├─► if fd == STDIN_FILENO (0)                         │
│       │       │                                             │
│       │       ▼                                             │
│       │   Try Uart_read() from ring buffer                  │
│       │       │                                             │
│       │       ├─► If data available: store in buf           │
│       │       │                                             │
│       │       └─► If no data: check RXNE flag directly      │
│       │               │                                     │
│       │               ▼                                     │
│       │           Read from USART->DR                       │
│       │               │                                     │
│       │               ▼                                     │
│       │           return bytes_read (0 if none)             │
│       │                                                     │
│       └─► else: return -ENOSYS                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_read_impl(int fd, uint8_t *buf, uint32_t len)
{
    if (buf == 0U) return -EINVAL;
    
    if (fd == STDIN_FILENO) {
        extern UART_HandleTypeDef huart2;
        uint32_t count = 0;
        
        if (len == 0) return 0;
        
        // Non-blocking read attempt
        int c = Uart_read(&huart2);
        if (c == -1) {
            // Fallback: direct hardware poll
            if (huart2.Instance->SR & USART_SR_RXNE) {
                c = (int)(huart2.Instance->DR & 0xFF);
            } else {
                return 0;  // No data available
            }
        }
        buf[count++] = (uint8_t)c;
        
        // Read additional available bytes
        while (count < len) {
            int d = Uart_read(&huart2);
            if (d == -1) {
                if (huart2.Instance->SR & USART_SR_RXNE) {
                    d = (int)(huart2.Instance->DR & 0xFF);
                } else break;
            }
            buf[count++] = (uint8_t)d;
        }
        return (int32_t)count;
    }
    
    return -ENOSYS;
}
```

**Notes:**
- Non-blocking implementation (returns 0 if no data)
- First tries ring buffer, then falls back to direct register access
- Maximum input size can be limited to 256 bytes per assignment spec

---

### 3. SYS___time

**Syscall Number:** `113` (defined in `syscall_def.h`)

**Purpose:** Return elapsed system time in milliseconds since boot

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  time_ms()  [userland wrapper]                              │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS___time, 0, 0, 0)                            │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(113, 0, 0, 0)                             │
│       │                                                     │
│       ▼                                                     │
│  sys_time_impl()                                            │
│       │                                                     │
│       ▼                                                     │
│  __getTime()  ──► return systick_ms_counter                 │
│       │                                                     │
│       ▼                                                     │
│  return milliseconds since boot                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_time_impl(void)
{
    return (int32_t)__getTime();
}
```

**File:** `src/kern/arch/cm4/cm4.c`

```c
static volatile uint32_t systick_ms_counter = 0;

uint32_t __getTime(void)
{
    return systick_ms_counter;
}

void SysTick_Handler(void)
{
    systick_ms_counter++;  // Incremented every 1ms
    // ... scheduler tick handling
}
```

**SysTick Configuration:**
- Configured to interrupt every 1ms
- `systick_ms_counter` incremented in `SysTick_Handler`
- Provides timing for delays, profiling, and scheduling

---

### 4. SYS_getpid

**Syscall Number:** `5` (defined in `syscall_def.h`)

**Purpose:** Return the task ID of the currently executing task

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  getpid()                                                   │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS_getpid, 0, 0, 0)                            │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(5, 0, 0, 0)                               │
│       │                                                     │
│       ▼                                                     │
│  sys_getpid_impl()                                          │
│       │                                                     │
│       ▼                                                     │
│  get_current_task()  ──► returns TCB pointer                │
│       │                                                     │
│       ▼                                                     │
│  return current->task_id                                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_getpid_impl(void)
{
    extern TCB_TypeDef* get_current_task(void);
    TCB_TypeDef *current = get_current_task();
    if (current != 0) {
        return (int32_t)current->task_id;
    }
    return 1;  // Fallback if no task running
}
```

**File:** `src/kern/lib/kern/schedule.c`

```c
TCB_TypeDef* get_current_task(void)
{
    return g_scheduler.current_task;
}
```

**Task Control Block (TCB) Structure:**

```c
typedef struct TCB {
    uint32_t task_id;           // Unique task identifier
    uint32_t psp;               // Process Stack Pointer
    TaskStatus status;          // RUNNING, READY, BLOCKED, KILLED
    uint8_t priority;           // Task priority
    struct TCB *next;           // Next task in queue
    // ... other fields
} TCB_TypeDef;
```

---

### 5. SYS_yield

**Syscall Number:** `120` (defined in `syscall_def.h`)

**Purpose:** Voluntarily relinquish CPU to allow next task to run

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  yield()                                                    │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS_yield, 0, 0, 0)                             │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(120, 0, 0, 0)                             │
│       │                                                     │
│       ▼                                                     │
│  sys_yield_impl()                                           │
│       │                                                     │
│       ▼                                                     │
│  task_yield()                                               │
│       │                                                     │
│       ▼                                                     │
│  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk   ◄── Set PendSV bit   │
│       │                                                     │
│       ▼                                                     │
│  return 0                                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼ (On exception exit, PendSV triggers)
┌─────────────────────────────────────────────────────────────┐
│ PendSV_Handler                                              │
│                                                             │
│  1. Save current task's context (R4-R11) to its stack       │
│  2. Save PSP to current task's TCB                          │
│  3. Select next task from ready queue                       │
│  4. Restore next task's PSP                                 │
│  5. Restore next task's context (R4-R11)                    │
│  6. Return with EXC_RETURN → resumes next task              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_yield_impl(void)
{
    task_yield();
    return 0;
}
```

**File:** `src/kern/lib/kern/schedule.c`

```c
void task_yield(void)
{
    /* Trigger PendSV to perform context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
```

**Why PendSV for Context Switch?**
- PendSV has lowest priority, ensuring all other interrupts complete first
- Deferred execution allows clean context switching
- Set via `ICSR.PENDSVSET` bit in System Control Block

---

### 6. SYS_reboot

**Syscall Number:** `119` (defined in `syscall_def.h`)

**Purpose:** Perform a software reset of the microcontroller

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  reboot()                                                   │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS_reboot, 0, 0, 0)                            │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(119, 0, 0, 0)                             │
│       │                                                     │
│       ▼                                                     │
│  sys_reboot_impl()                                          │
│       │                                                     │
│       ▼                                                     │
│  NVIC_SystemReset()                                         │
│       │                                                     │
│       ▼                                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Write to SCB->AIRCR:                                │    │
│  │   - VECTKEY = 0x5FA (authorization key)             │    │
│  │   - SYSRESETREQ = 1 (request system reset)          │    │
│  └─────────────────────────────────────────────────────┘    │
│       │                                                     │
│       ▼                                                     │
│  __DSB()  ──► Data Synchronization Barrier                  │
│       │                                                     │
│       ▼                                                     │
│  *** SYSTEM RESET *** (Jumps to Reset_Handler)              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_reboot_impl(void)
{
    NVIC_SystemReset();
    return 0;  // Never reached
}
```

**NVIC_SystemReset() (CMSIS function):**

```c
static inline void NVIC_SystemReset(void)
{
    __DSB();  // Ensure all outstanding memory accesses complete
    
    SCB->AIRCR = (0x5FA << SCB_AIRCR_VECTKEY_Pos) |  // Magic key
                 SCB_AIRCR_SYSRESETREQ_Msk;           // Reset request
    
    __DSB();  // Ensure write completes
    
    while(1);  // Wait for reset
}
```

**AIRCR Register:**
- `VECTKEY` (bits 31:16): Must write 0x5FA to authorize changes
- `SYSRESETREQ` (bit 2): Requests system reset
- Reset clears all registers and peripherals to default state
- Flash memory contents preserved

---

### 7. SYS__exit

**Syscall Number:** `3` (defined in `syscall_def.h`)

**Purpose:** Terminate the current task and yield CPU to next task

#### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ USER SPACE                                                  │
│                                                             │
│  _exit(status)                                              │
│       │                                                     │
│       ▼                                                     │
│  svc_invoke(SYS__exit, status, 0, 0)                        │
│       │                                                     │
│       ▼                                                     │
│  SVC #0  ─────────────────────────────────────────────────► │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│ KERNEL SPACE (Privileged)                                   │
│                                                             │
│  SVCall_Handler → SVCall_Handler_C                          │
│       │                                                     │
│       ▼                                                     │
│  syscall_dispatch(3, status, 0, 0)                          │
│       │                                                     │
│       ▼                                                     │
│  sys_exit_impl(status)                                      │
│       │                                                     │
│       ▼                                                     │
│  get_current_task() ──► Get current TCB                     │
│       │                                                     │
│       ▼                                                     │
│  current->status = TASK_STATUS_KILLED  ◄── Mark terminated  │
│       │                                                     │
│       ▼                                                     │
│  task_yield() ──► Trigger PendSV for context switch         │
│       │                                                     │
│       ▼                                                     │
│  while(1) { __WFI(); }  ◄── Fallback if no other tasks      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Key Implementation Details

**File:** `src/kern/syscall/syscall.c`

```c
static int32_t sys_exit_impl(int code)
{
    (void)code;
    
    // Step 1: Mark task as terminated
    extern TCB_TypeDef* get_current_task(void);
    TCB_TypeDef *current = get_current_task();
    if (current != 0) {
        current->status = TASK_STATUS_KILLED;
    }
    
    // Step 2: Yield CPU to scheduler
    task_yield();
    
    // Fallback: If no other tasks, enter low-power wait
    while (1) {
        __WFI();  // Wait For Interrupt
    }
    return 0;
}
```

**Task Status Transitions:**

```
                    ┌─────────┐
      create_task() │  READY  │◄─────────────────┐
                    └────┬────┘                  │
                         │ scheduler picks       │
                         ▼                       │
                    ┌─────────┐      yield()     │
                    │ RUNNING │──────────────────┤
                    └────┬────┘                  │
                         │ _exit()               │
                         ▼                       │
                    ┌─────────┐                  │
                    │ KILLED  │  (removed from   │
                    └─────────┘   ready queue)   │
```

---

## File Structure

```
src/
├── kern/
│   ├── include/
│   │   ├── kern/
│   │   │   ├── syscall_def.h    ← Syscall numbers (SYS_read=50, etc.)
│   │   │   ├── schedule.h       ← TCB structure, scheduler APIs
│   │   │   └── kunistd.h        ← STDIN/STDOUT/STDERR definitions
│   │   └── syscall.h            ← syscall_dispatch() prototype
│   │
│   ├── syscall/
│   │   └── syscall.c            ← Syscall implementations
│   │
│   ├── lib/kern/
│   │   └── schedule.c           ← Scheduler, task_yield(), get_current_task()
│   │
│   └── arch/
│       ├── cm4/
│       │   └── cm4.c            ← __getTime(), SysTick_Handler
│       └── stm32f446re/
│           └── sys_lib/
│               └── stm32_startup.c  ← SVCall_Handler, vector table
│
└── userland/
    ├── include/
    │   ├── unistd.h             ← User API (read, write, getpid, etc.)
    │   └── times.h              ← time_ms() declaration
    └── utils/
        ├── unistd.c             ← svc_invoke() and syscall wrappers
        └── interactive_syscall_menu.c  ← Test menu
```

---

## Testing

### Interactive Syscall Menu

The `interactive_syscall_menu()` function in userland provides a test interface:

```
================================================
SYSCALL TESTING MENU
================================================
Select a syscall to test:
1) SYS_write   - Write to stdout/stderr
2) SYS_read    - Read from stdin
3) SYS___time  - Get system time
4) SYS_getpid  - Get process ID
5) SYS_yield   - Yield CPU to scheduler
6) SYS_reboot  - Reboot system (CAUTION!)
7) SYS__exit   - Exit and halt (CAUTION!)
8) Debug Info  - Show handler debug counters
9) Raw Key Monitor (diagnostic)
0) Exit        - Return to main
```

### Debug Counters

Global variables track syscall activity:

```c
volatile uint32_t g_svcall_count;        // Total SVC handler invocations
volatile uint32_t g_last_callno;         // Last syscall number
volatile int32_t  g_last_retval;         // Last return value
volatile uint32_t g_syscall_dispatch_count;
volatile uint32_t g_last_syscall_id;
volatile uint32_t g_last_arg1;
volatile int32_t  g_last_syscall_ret;
```

---

## Summary Table

| Syscall | Number | Arguments | Return Value | Key Function |
|---------|--------|-----------|--------------|--------------|
| `SYS_write` | 55 | fd, buf, len | bytes written | `noIntWrite()` |
| `SYS_read` | 50 | fd, buf, len | bytes read | `Uart_read()` |
| `SYS___time` | 113 | none | milliseconds | `__getTime()` |
| `SYS_getpid` | 5 | none | task_id | `get_current_task()` |
| `SYS_yield` | 120 | none | 0 | `task_yield()` → PendSV |
| `SYS_reboot` | 119 | none | (no return) | `NVIC_SystemReset()` |
| `SYS__exit` | 3 | status | (no return) | Set KILLED + yield |

---

*Document generated for CSE-3211 Operating System Lab, Assignment 02*
