# 📚 DUOS System Call Implementation Manual
## A Complete Beginner's Guide

**For:** CSE-3211 Operating System Lab, Assignment 02  
**University of Dhaka, Department of Computer Science and Engineering**

---

## Table of Contents

1. [Who Is This Manual For?](#chapter-1-who-is-this-manual-for)
2. [Understanding the Basics](#chapter-2-understanding-the-basics)
   - [What Is an Operating System?](#21-what-is-an-operating-system)
   - [What Is DUOS?](#22-what-is-duos)
   - [What Is a Microcontroller?](#23-what-is-a-microcontroller)
   - [What Is a System Call?](#24-what-is-a-system-call)
3. [Privileged vs.  Unprivileged Modes](#chapter-3-privileged-vs-unprivileged-modes)
   - [The Two Modes of Operation](#31-the-two-modes-of-operation)
   - [Why Two Modes?](#32-why-two-modes)
   - [Visual Representation](#33-visual-representation)
4. [The SVC Exception Mechanism](#chapter-4-the-svc-exception-mechanism)
   - [What Is SVC?](#41-what-is-svc)
   - [What Is an Exception?](#42-what-is-an-exception)
   - [The Complete Journey of a System Call](#43-the-complete-journey-of-a-system-call)
5. [Understanding the Stack](#chapter-5-understanding-the-stack)
   - [What Is a Stack?](#51-what-is-a-stack)
   - [Two Types of Stacks in ARM Cortex-M4](#52-two-types-of-stacks-in-arm-cortex-m4)
   - [The Stack Frame on Exception Entry](#53-the-stack-frame-on-exception-entry)
6. [Code Walkthrough](#chapter-6-code-walkthrough)
   - [File Organization](#61-file-organization)
   - [syscall_def. h - The Phonebook](#62-syscall_defh---the-phonebook)
   - [unistd.c - The User-Space Wrappers](#63-unistdc---the-user-space-wrappers)
   - [SVCall_Handler - The Exception Entry Point](#64-svcall_handler---the-exception-entry-point)
   - [syscall_dispatch - The Router](#65-syscall_dispatch---the-router)
7. [Detailed Syscall Implementations](#chapter-7-detailed-syscall-implementations)
   - [SYS_write](#71-sys_write-number-55---writing-to-the-screen)
   - [SYS_read](#72-sys_read-number-50---reading-from-keyboard)
   - [SYS___time](#73-sys___time-number-113---getting-system-time)
   - [SYS_getpid](#74-sys_getpid-number-5---getting-process-id)
   - [SYS_yield](#75-sys_yield-number-120---voluntarily-giving-up-cpu)
   - [SYS_reboot](#76-sys_reboot-number-119---rebooting-the-system)
   - [SYS__exit](#77-sys__exit-number-3---terminating-a-task)
8. [Summary Table](#chapter-8-summary-table)
9. [Glossary](#chapter-9-glossary)
10. [Quick Reference](#chapter-10-quick-reference)

---

## Chapter 1: Who Is This Manual For? 

This manual is written for **complete beginners** who have never worked with operating systems, embedded systems, or system programming before. We'll explain every concept from the ground up, using simple analogies and step-by-step explanations.

**Prerequisites:**
- Basic understanding of C programming
- Familiarity with concepts like variables, functions, and pointers
- No prior operating system knowledge required! 

---

## Chapter 2: Understanding the Basics

### 2.1 What Is an Operating System? 

Think of an operating system (OS) like a **manager in a building**. Just as a building manager: 

| Building Manager | Operating System |
|------------------|------------------|
| Controls who can access different rooms | Controls which programs can access hardware |
| Manages shared resources like elevators and electricity | Manages memory, CPU time, and devices |
| Coordinates activities between different tenants | Coordinates multiple programs running simultaneously |

An operating system provides: 
- **Security**: Prevents programs from interfering with each other
- **Resource Management**: Fairly distributes CPU, memory, and devices
- **Abstraction**: Hides complex hardware details from programmers

### 2.2 What Is DUOS?

**DUOS** (Dhaka University Operating System) is a **simple, educational operating system** designed for learning.  It runs on an **STM32F446RE microcontroller** – a small computer chip commonly used in embedded devices like: 
- Smart watches
- Car dashboards
- Industrial sensors
- Home automation systems

> 💡 **Analogy**: Think of it like learning to drive with a go-kart before driving a real car. DUOS is simpler than Linux or Windows, but it teaches you the same fundamental concepts. 

### 2.3 What Is a Microcontroller? 

A **microcontroller** is a tiny computer on a single chip. The **STM32F446RE** contains: 

| Component | Description | Size |
|-----------|-------------|------|
| **CPU (Cortex-M4)** | The "brain" that executes instructions | 180 MHz |
| **RAM** | Temporary memory for running programs | 128 KB |
| **Flash Memory** | Permanent storage for your code | 512 KB |
| **Peripherals** | Built-in hardware (timers, UART, GPIO) | Various |

> 💡 **Analogy**: If a desktop computer is a full house with separate rooms (CPU, memory, storage, etc.), a microcontroller is a studio apartment where everything is in one space.

### 2.4 What Is a System Call? 

Imagine you're in a **secure office building**. You (an employee) can't just walk into the server room yourself. Instead, you fill out a request form and give it to the IT administrator, who then performs the action for you. 

A **system call (syscall)** works exactly the same way: 

| Real World | Computer World |
|------------|----------------|
| You (employee) | Your program (unprivileged) |
| IT Administrator | Operating system kernel (privileged) |
| Request form | System call |
| Server room access | Hardware access |

**Why do we need this?**

1. **Security**:  Prevents buggy/malicious programs from crashing the system
2. **Fairness**: Ensures all programs share resources properly
3. **Abstraction**: Simplifies programming (you don't need to know hardware details)

---

## Chapter 3: Privileged vs. Unprivileged Modes

### 3.1 The Two Modes of Operation

The ARM Cortex-M4 processor has two main operating modes:

#### Unprivileged (User) Mode

- Where normal user programs run
- **Limited powers**: Cannot directly access hardware
- **Restricted memory**: Can only access its own designated memory
- **Cannot modify system settings**

> 💡 **Analogy**: Like being a regular employee who can only access their desk and common areas. 

#### Privileged (Kernel) Mode

- Where the operating system kernel runs
- **Full powers**: Can access all hardware directly
- **Full memory access**: Can access any memory location
- **Can modify system settings**

> 💡 **Analogy**: Like being the building's super administrator with a master key to every room. 

### 3.2 Why Two Modes? 

Imagine if any program could: 
- ❌ Turn off the computer whenever it wanted
- ❌ Read other programs' private data (like passwords)
- ❌ Overwrite critical system files

This would be chaos! The two-mode system creates a **protective barrier** between user programs and the critical system components.

### 3.3 Visual Representation

```
┌───────────────────────────────────────────────────────────────┐
│                    YOUR COMPUTER / MICROCONTROLLER            │
│                                                               │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │           UNPRIVILEGED MODE (User Space)                │  │
│  │                                                         │  │
│  │   ┌─────────┐  ┌─────────┐  ┌─────────┐                │  │
│  │   │ App 1   │  │ App 2   │  │ App 3   │                │  │
│  │   │(Game)   │  │(Editor) │  │(Browser)│                │  │
│  │   └────┬────┘  └────┬────┘  └────┬────┘                │  │
│  │        │            │            │                      │  │
│  └────────┼────────────┼────────────┼──────────────────────┘  │
│           │            │            │                         │
│           ▼            ▼            ▼                         │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ ═════════���═════ SYSTEM CALL INTERFACE ═══════════════  │  │
│  │         (The "Request Form Submission Counter")         │  │
│  └────────────────────────────┬────────────────────────────┘  │
│                               │                               │
│  ┌────────────────────────────▼────────────────────────────┐  │
│  │             PRIVILEGED MODE (Kernel Space)              │  │
│  │                                                         │  │
│  │   ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │  │
│  │   │ Scheduler│  │  Memory  │  │   Device Drivers     │ │  │
│  │   │          │  │ Manager  │  │ (UART, Timer, etc.)  │ │  │
│  │   └──────────┘  └──────────┘  └──────────────────────┘ │  │
│  │                                                         │  │
│  └─────────────────────────────────────────────────────────┘  │
│                               │                               │
│                               ▼                               │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │                     HARDWARE                            │  │
│  │   [CPU] [Memory] [UART] [Timer] [GPIO] [Peripherals]    │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

---

## Chapter 4: The SVC Exception Mechanism

### 4.1 What Is SVC?

**SVC** stands for **Supervisor Call**. It's a special CPU instruction that: 

1. Switches from unprivileged mode to privileged mode
2. Jumps to a special piece of code (handler) in the kernel
3. Allows the kernel to perform a service for the user program

> 💡 **Analogy**: It's like pressing the "Call Staff" button at a hotel. You (the guest) can't go into the kitchen, but pressing the button summons a staff member who can. 

### 4.2 What Is an Exception?

An **exception** is any event that interrupts normal program execution. Think of it like: 
- Your phone ringing while you're reading a book (you stop, answer, then resume reading)
- A fire alarm in a building (everyone stops their work and follows the emergency procedure)

In ARM Cortex-M4, there are several types of exceptions: 

| Exception | What Triggers It | Priority |
|-----------|------------------|----------|
| Reset | Power on or reset button | Highest |
| NMI | Non-maskable interrupt (critical) | Very High |
| HardFault | Serious error (illegal instruction) | High |
| **SVCall** | **SVC instruction (system call)** | **Configurable** |
| PendSV | Pending supervisor request (context switch) | Low |
| SysTick | System timer tick | Configurable |

### 4.3 The Complete Journey of a System Call

Let's trace what happens when a user program calls `write()` to print "Hello" to the screen:

#### STEP 1: User Program Calls write()

```c
// Your Code: 
write(1, "Hello", 5);
```

**What happens**:  Your program wants to send "Hello" to the screen.  The function `write()` is just a "wrapper" that prepares the request. 

#### STEP 2: Wrapper Function Prepares Arguments

The `write()` function internally:
- Puts syscall number (55) in register R0
- Puts file descriptor (1) in register R1
- Puts buffer address in register R2
- Puts length (5) in register R3

```
┌───────────────────────��─────────────┐
│  CPU REGISTERS (Before SVC)         │
│  ┌─────┬─────────────────────────┐  │
│  │ R0  │ 55 (syscall ID)         │  │
│  │ R1  │ 1 (stdout)              │  │
│  │ R2  │ 0x20001000 (buf addr)   │  │
│  │ R3  │ 5 (length)              │  │
│  └─────┴─────────────────────────┘  │
└─────────────────────────────────────┘
```

#### STEP 3: Execute SVC Instruction

The wrapper then executes: 
```asm
SVC #0
```

This is the "magic button" that switches to privileged mode! 

#### STEP 4: Hardware Automatically Saves Context

The CPU **AUTOMATICALLY** (hardware does this, not software):
1. Stops the user program
2. Saves 8 registers onto the stack: 
   - R0, R1, R2, R3 (arguments)
   - R12 (scratch register)
   - LR (return address)
   - PC (program counter - where we were)
   - xPSR (processor status)
3. Switches to privileged mode
4. Jumps to SVCall_Handler

```
┌─────────────────────────────────────┐
│  STACK (After hardware auto-save)   │
│                                     │
│  ┌──────────┬─────────────────────┐ │
│  │ xPSR     │ Processor status    │ │ ← Higher address
│  ├──────────┼─────────────────────┤ │
│  │ PC       │ Where to return     │ │
│  ├──────────┼─────────────────────┤ │
│  │ LR       │ Link register       │ │
│  ├──────────┼─────────────────────┤ │
│  │ R12      │ Scratch             │ │
│  ├──────────┼─────���───────────────┤ │
│  │ R3       │ 5 (length)          │ │
│  ├──────────┼─────────────────────┤ │
│  │ R2       │ Buffer address      │ │
│  ├──────────┼─────────────────────┤ │
│  │ R1       │ 1 (fd)              │ │
│  ├──────────┼─────────────────────┤ │
│  │ R0       │ 55 (syscall ID)     │ │ ← Lower address (stack_ptr)
│  └──────────┴─────────────────────┘ │
└─────────────────────────────────────┘
```

#### STEP 5: SVCall_Handler (Assembly) Runs

This code determines which stack the registers were saved on and passes a pointer to the C handler. 

#### STEP 6: SVCall_Handler_C (C Code) Runs

- Extracts syscall ID and arguments from the stack
- Calls `syscall_dispatch()` to route to the right handler

#### STEP 7: syscall_dispatch() Routes the Request

Uses a switch statement to call the appropriate function:
- ID 55 → `sys_write_impl()`
- ID 50 → `sys_read_impl()`
- ID 5 → `sys_getpid_impl()`
- etc.

#### STEP 8: Specific Syscall Handler Executes

`sys_write_impl()` runs: 
- Validates the file descriptor (is it stdout?)
- Loops through the buffer
- Sends each character to the UART hardware
- Returns number of bytes written (5)

#### STEP 9: Return Value Placed on Stack

The return value (5) is written to where R0 was saved on the stack.  This way, when the user program resumes, it will see 5 in R0.

#### STEP 10: Exception Return

A special return instruction (EXC_RETURN) triggers: 
- Hardware restores all 8 saved registers
- Switches back to unprivileged mode
- User program resumes right after the SVC instruction

#### STEP 11: User Program Gets Result

The `write()` wrapper returns 5 to your program. 

```c
// Your program sees: write() returned 5 (success!)
```

---

## Chapter 5: Understanding the Stack

### 5.1 What Is a Stack? 

A **stack** is a special area of memory that works like a **stack of plates**: 

- **Push**: Put a new plate on top
- **Pop**: Take the top plate off
- **LIFO**:  Last In, First Out

In computers, we use stacks for:
- Storing function return addresses
- Saving register values temporarily
- Passing arguments between functions

```
STACK OPERATIONS:
═════════════════

PUSH (add item):          POP (remove item):
                          
    ┌─────┐                   ┌─────┐
    │  C  │ ← New item        │  C  │ ← Removed
    ├─────┤                   ├─────┤
    │  B  │                   │  B  │ ← New top
    ├─────┤                   ├─────┤
    │  A  │                   │  A  │
    └─────┘                   └─────┘
```

### 5.2 Two Types of Stacks in ARM Cortex-M4

ARM Cortex-M4 has **two** stack pointers: 

| Stack Pointer | Name | Used By |
|---------------|------|---------|
| **MSP** | Main Stack Pointer | Exception handlers, kernel code |
| **PSP** | Process Stack Pointer | User threads/tasks |

**Why two stacks?**

This separation protects the kernel stack from user programs. Even if a user program corrupts its own stack (PSP), the kernel stack (MSP) remains intact.

```
MEMORY LAYOUT (Simplified)
════════════════════════════

Higher Addresses
        │
        ▼
┌───────────────────────┐
│                       │
│   Main Stack (MSP)    │  ← Used by kernel/exception handlers
│     (grows down)      │
│         │             │
│         ▼             │
├───────────────────────┤
│                       │
│      Empty Space      │
│                       │
├───────────────────────┤
│         ▲             │
│         │             │
│  Process Stack (PSP)  │  ← Used by user tasks
│     (grows down)      │
│                       │
├───────────────────────┤
│                       │
│   Heap (dynamic)      │  ← malloc() uses this
│                       │
├───────────────────────┤
│                       │
│   Global Variables    │
│   (data section)      │
│                       │
├───────────────────────┤
│                       │
│   Code (text)         │  ← Your program instructions
│                       │
└───────────────────────┘
        ▲
        │
Lower Addresses
```

### 5.3 The Stack Frame on Exception Entry

When an exception (like SVC) occurs, the CPU automatically saves 8 registers onto the stack.  This is called the **exception stack frame**:

```c
// Stack frame structure (what's saved automatically by hardware)
struct exception_frame {
    uint32_t r0;      // offset 0  - Syscall ID / Return value
    uint32_t r1;      // offset 4  - Argument 1
    uint32_t r2;      // offset 8  - Argument 2
    uint32_t r3;      // offset 12 - Argument 3
    uint32_t r12;     // offset 16 - Scratch register
    uint32_t lr;      // offset 20 - Link Register (return address)
    uint32_t pc;      // offset 24 - Program Counter (next instruction)
    uint32_t xpsr;    // offset 28 - Processor Status Register
};
```

**This is why we can access arguments using array indices:**

| Array Index | Register | Content |
|-------------|----------|---------|
| `stack_ptr[0]` | R0 | Syscall ID |
| `stack_ptr[1]` | R1 | First argument |
| `stack_ptr[2]` | R2 | Second argument |
| `stack_ptr[3]` | R3 | Third argument |
| `stack_ptr[4]` | R12 | Scratch register |
| `stack_ptr[5]` | LR | Link register |
| `stack_ptr[6]` | PC | Program counter |
| `stack_ptr[7]` | xPSR | Processor status |

---

## Chapter 6: Code Walkthrough

### 6.1 File Organization

The DUOS syscall implementation is organized into these key files:

```
src/
├── kern/                           ← KERNEL (Privileged) Code
│   ├── include/
│   │   ├── kern/
│   │   │   └── syscall_def.h       ← Syscall ID numbers (55, 50, etc.)
│   │   └── syscall.h               ← Function prototypes
│   │
│   ├── syscall/
│   │   └── syscall. c               ← Syscall implementations
│   │
│   └── arch/stm32f446re/
│       └── sys_lib/
│           └── stm32_startup.c     ← SVCall_Handler (assembly + C)
│
└── userland/                        ← USER (Unprivileged) Code
    └── utils/
        └── unistd.c                 ← User-space wrappers (write, read, etc.)
```

### 6.2 syscall_def.h - The Phonebook

This file assigns a **unique number** to each system call.  Think of it like a phonebook where each service has an extension number:

```c
// Process-related syscalls
#define SYS__exit        3    // Terminate process
#define SYS_getpid       5    // Get process ID

// File/IO-related syscalls
#define SYS_read         50   // Read from file/device
#define SYS_write        55   // Write to file/device

// Time-related syscalls
#define SYS___time       113  // Get system time

// System control syscalls
#define SYS_reboot       119  // Reboot the system
#define SYS_yield        120  // Give up CPU voluntarily
```

**Why use numbers instead of names?**

- Numbers are faster to compare in code (simple integer comparison)
- They take less space (2 bytes vs.  a full string)
- They're language-independent (works the same in assembly and C)

### 6.3 unistd.c - The User-Space Wrappers

These functions provide a **friendly interface** for user programs.  Instead of dealing with registers and assembly, you just call a normal C function:

```c
// The "magic" function that triggers the syscall
static inline int svc_invoke(uint32_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
    // Put syscall number in R0
    register uint32_t r0 __asm__("r0") = callno;
    // Put arguments in R1, R2, R3
    register uint32_t r1 __asm__("r1") = a1;
    register uint32_t r2 __asm__("r2") = a2;
    register uint32_t r3 __asm__("r3") = a3;

    // Execute SVC instruction - this triggers the exception! 
    __asm volatile ("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");

    // After returning, r0 contains the result
    return (int)r0;
}

// User-friendly wrapper for write()
ssize_t write(int fd, const void *buf, size_t count)
{
    // Call svc_invoke with syscall ID 55 (SYS_write)
    return (ssize_t)svc_invoke(SYS_write, (uint32_t)fd, (uint32_t)buf, (uint32_t)count);
}

// User-friendly wrapper for read()
ssize_t read(int fd, void *buf, size_t count)
{
    return (ssize_t)svc_invoke(SYS_read, (uint32_t)fd, (uint32_t)buf, (uint32_t)count);
}
```

**Breaking down the assembly:**

| Code | Explanation |
|------|-------------|
| `__asm__("r0")` | Tells the compiler to use a specific CPU register |
| `__asm volatile ("svc 0" ...)` | Executes the SVC instruction |
| `"+r"(r0)` | R0 is both input (syscall ID) and output (return value) |
| `"memory"` | Tells the compiler that memory may have changed |

### 6.4 SVCall_Handler - The Exception Entry Point

This is the first code that runs when an SVC instruction is executed:

```c
// Assembly handler - determines which stack was used
__attribute__((naked)) void SVCall_Handler(void)
{
    __asm volatile (
        "tst   lr, #4        \n"  // Test bit 2 of LR
        "ite   eq            \n"  // If-Then-Else conditional
        "mrseq r0, msp       \n"  // If bit 2 is 0:  use MSP
        "mrsne r0, psp       \n"  // If bit 2 is 1: use PSP
        "b     SVCall_Handler_C \n"  // Jump to C handler
    );
}

// C handler - processes the syscall
void SVCall_Handler_C(uint32_t *stack_ptr)
{
    // Extract syscall ID and arguments from the saved registers
    uint16_t callno = (uint16_t)(stack_ptr[0]);  // R0 = syscall ID
    uint32_t a1 = stack_ptr[1];                   // R1 = arg1
    uint32_t a2 = stack_ptr[2];                   // R2 = arg2
    uint32_t a3 = stack_ptr[3];                   // R3 = arg3

    // Route to the appropriate handler
    int32_t ret = syscall_dispatch(callno, a1, a2, a3);

    // Put return value where R0 was saved
    // When we return, hardware will restore this into R0
    stack_ptr[0] = (uint32_t)ret;
}
```

**Understanding EXC_RETURN:**

When an exception occurs, the **Link Register (LR)** is loaded with a special value called **EXC_RETURN**.  Bit 2 of this value tells us which stack was being used:

| Bit 2 Value | Stack Used | Description |
|-------------|------------|-------------|
| 0 | MSP | Main Stack Pointer was used |
| 1 | PSP | Process Stack Pointer was used |

This is important because we need to know where the saved registers are! 

### 6.5 syscall_dispatch - The Router

This function routes each syscall to its specific implementation:

```c
int32_t syscall_dispatch(uint16_t callno, uint32_t a1, uint32_t a2, uint32_t a3)
{
    int32_t rv;

    switch (callno)
    {
        case SYS_read:    // 50
            rv = sys_read_impl((int)a1, (uint8_t*)a2, a3);
            break;

        case SYS_write:    // 55
            rv = sys_write_impl((int)a1, (const uint8_t*)a2, a3);
            break;

        case SYS_reboot:  // 119
            rv = sys_reboot_impl();
            break;

        case SYS__exit:   // 3
            rv = sys_exit_impl((int)a1);
            break;

        case SYS_getpid:  // 5
            rv = sys_getpid_impl();
            break;

        case SYS___time:  // 113
            rv = sys_time_impl();
            break;

        case SYS_yield:   // 120
            rv = sys_yield_impl();
            break;

        default:
            rv = -ENOSYS;  // Error:  syscall not implemented
            break;
    }

    return rv;
}
```

---

## Chapter 7: Detailed Syscall Implementations

### 7.1 SYS_write (Number:  55) - Writing to the Screen

**Purpose**: Send data to a file or device (in our case, the serial terminal)

**Arguments**: 

| Parameter | Type | Description |
|-----------|------|-------------|
| `fd` | int | File descriptor (1 = stdout, 2 = stderr) |
| `buf` | const uint8_t* | Address of the data buffer to write |
| `len` | uint32_t | Number of bytes to write |

**Returns**: Number of bytes actually written, or negative error code

```c
static int32_t sys_write_impl(int fd, const uint8_t *buf, uint32_t len)
{
    // Safety check: make sure buffer pointer is valid
    if (buf == 0U) return -EINVAL;  // Error: invalid argument

    // We only support writing to stdout (1) or stderr (2)
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        // Get the UART (serial port) handle
        extern UART_HandleTypeDef huart2;

        // Send each byte one at a time
        for (uint32_t i = 0; i < len; i++) {
            noIntWrite(&huart2, (char)buf[i]);  // Send one character
        }
        return (int32_t)len;  // Return number of bytes written
    }

    // Any other file descriptor is not supported
    return -ENOSYS;  // Error: function not implemented
}
```

#### What is UART?

**UART** (Universal Asynchronous Receiver-Transmitter) is a hardware component that converts data into serial format for transmission.  It's how the microcontroller talks to your computer through a USB cable. 

#### Why noIntWrite() instead of regular write? 

When we're inside an exception handler, interrupts with lower priority are blocked. The normal UART write function uses interrupts and would wait forever (deadlock). `noIntWrite()` uses **polling** – it directly checks if the hardware is ready and sends data without waiting for interrupts.

```
HOW noIntWrite() WORKS:
═══════════════════════

1. Check if TX (Transmit) buffer is empty
   ┌─────────────────────┐
   │ USART Status Reg    │
   │ Is TXE bit = 1?     │──► No → Wait (loop)
   │                     │──► Yes → Continue
   └─────────────────────┘

2. Write character to Data Register
   ┌─────────────────────┐
   │ USART Data Register │ ← Write character here
   └─────────────────────┘

3. Hardware transmits the byte automatically

4.  Repeat for next character
```

---

### 7.2 SYS_read (Number:  50) - Reading from Keyboard

**Purpose**:  Read data from a file or device (in our case, keyboard input via serial)

**Arguments**:

| Parameter | Type | Description |
|-----------|------|-------------|
| `fd` | int | File descriptor (0 = stdin for keyboard) |
| `buf` | uint8_t* | Address of buffer to store the data |
| `len` | uint32_t | Maximum number of bytes to read |

**Returns**:  Number of bytes actually read (0 if no data available), or negative error code

```c
static int32_t sys_read_impl(int fd, uint8_t *buf, uint32_t len)
{
    // Safety check
    if (buf == 0U) return -EINVAL;

    // Only support reading from stdin (keyboard)
    if (fd == STDIN_FILENO) {
        extern UART_HandleTypeDef huart2;
        uint32_t count = 0;

        if (len == 0) return 0;  // Nothing to read

        // Try to get the first character
        int c = Uart_read(&huart2);  // Try ring buffer first

        if (c == -1) {
            // Ring buffer empty, try direct hardware read
            if (huart2.Instance->SR & USART_SR_RXNE) {
                // Data available in hardware register
                c = (int)(huart2.Instance->DR & 0xFF);
            } else {
                return 0;  // No data available at all
            }
        }

        buf[count++] = (uint8_t)c;  // Store first character

        // Try to read more characters if available
        while (count < len) {
            int d = Uart_read(&huart2);
            if (d == -1) {
                if (huart2.Instance->SR & USART_SR_RXNE) {
                    d = (int)(huart2.Instance->DR & 0xFF);
                } else {
                    break;  // No more data
                }
            }
            buf[count++] = (uint8_t)d;
        }

        return (int32_t)count;  // Return number of bytes read
    }

    return -ENOSYS;
}
```

#### What is a Ring Buffer? 

A **ring buffer** (also called circular buffer) is like a conveyor belt for data. When the UART receives characters, they're stored in this buffer. The program can read them later at its own pace. 

```
RING BUFFER VISUALIZATION:
═══════════════════════════

   Write pointer (where new data goes)
        ↓
   ┌───┬───┬───┬───┬───┬───┬───┬───┐
   │ H │ e │ l │ l │ o │   │   │   │
   └───┴───┴───┴───┴───┴───┴───┴───┘
   ↑
   Read pointer (where we read from)

When we read:  "H" is removed, read pointer moves right
When we write: new char added at write pointer
When pointers meet: buffer is empty or full
```

---

### 7.3 SYS___time (Number: 113) - Getting System Time

**Purpose**:  Get the number of milliseconds since the system booted

**Arguments**:  None

**Returns**:  Milliseconds since boot (as a 32-bit integer)

```c
static int32_t sys_time_impl(void)
{
    return (int32_t)__getTime();  // Returns systick_ms_counter
}
```

#### How does __getTime() work?

The ARM Cortex-M4 has a built-in timer called **SysTick**. It's configured to generate an interrupt every 1 millisecond. Each time that interrupt fires, we increment a counter: 

```c
// This variable counts milliseconds
static volatile uint32_t systick_ms_counter = 0;

// This function runs every 1ms
void SysTick_Handler(void)
{
    systick_ms_counter++;  // Increment counter
    // Also handles scheduler timing... 
}

// This returns the current count
uint32_t __getTime(void)
{
    return systick_ms_counter;
}
```

#### What is `volatile`?

The `volatile` keyword tells the compiler:  "This variable can change at any time (by hardware or interrupt), so always read it fresh from memory instead of using a cached value."

---

### 7.4 SYS_getpid (Number:  5) - Getting Process ID

**Purpose**: Get the ID of the currently running task

**Arguments**: None

**Returns**: Task ID number

```c
static int32_t sys_getpid_impl(void)
{
    // Get pointer to current task's control block
    extern TCB_TypeDef* get_current_task(void);
    TCB_TypeDef *current = get_current_task();

    if (current != 0) {
        return (int32_t)current->task_id;  // Return the task's ID
    }

    return 1;  // Fallback if no task running
}
```

#### What is a Task Control Block (TCB)?

A **TCB** is a data structure that holds all information about a task (similar to a process in larger operating systems):

```c
typedef struct TCB {
    uint32_t task_id;    // Unique identifier (1, 2, 3, ...)
    uint32_t psp;        // This task's stack pointer
    TaskStatus status;   // RUNNING, READY, BLOCKED, or KILLED
    uint8_t priority;    // Higher = more important
    struct TCB *next;    // Pointer to next task in queue
    // ... other fields
} TCB_TypeDef;
```

> 💡 **Analogy**: Think of TCB as an employee badge that contains:
> - Employee ID
> - Current location (where they're working)
> - Status (working, on break, left for the day)
> - Priority level

---

### 7.5 SYS_yield (Number: 120) - Voluntarily Giving Up CPU

**Purpose**: Allow the current task to voluntarily give up the CPU so other tasks can run

**Arguments**: None

**Returns**: 0 (success)

```c
static int32_t sys_yield_impl(void)
{
    task_yield();  // Trigger a context switch
    return 0;
}

// In schedule. c:
void task_yield(void)
{
    // Set the PendSV pending bit
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
```

#### What is PendSV?

**PendSV** (Pendable Supervisor Call) is a special exception used for context switching.  Unlike SVCall, which runs immediately, PendSV is "pending" – it waits until all other higher-priority exceptions are done.

**Why use PendSV for context switching?**

1. It has the lowest priority, so it won't interrupt other important work
2. It can be safely triggered from within another exception (like SVCall)
3. It provides a clean point for saving/restoring task context

```
CONTEXT SWITCH FLOW:
═══════════════════

Task A running                    Task B waiting
    │                                  │
    ▼                                  │
yield() called                         │
    │                                  │
    ▼                                  │
SVC exception                          │
    │                                  │
    ▼                                  │
Set PendSV pending                     │
    │                                  │
    ▼                                  │
Return from SVC                        │
    │                                  │
    ▼                                  │
PendSV runs (low priority)             │
    │                                  │
    ▼                                  ▼
Save Task A's registers ────────► Load Task B's registers
    │                                  │
    ▼                                  ▼
Task A paused                     Task B runs! 
```

---

### 7.6 SYS_reboot (Number: 119) - Rebooting the System

**Purpose**: Restart the entire microcontroller

**Arguments**: None

**Returns**: Never returns (system resets)

```c
static int32_t sys_reboot_impl(void)
{
    NVIC_SystemReset();  // This function never returns! 
    return 0;  // Never reached
}
```

#### What does NVIC_SystemReset() do?

NVIC stands for **Nested Vectored Interrupt Controller** – the hardware that manages exceptions and interrupts. The reset is performed by writing to a special register:

```c
// This is the CMSIS (standard ARM library) implementation: 
static inline void NVIC_SystemReset(void)
{
    __DSB();  // Data Synchronization Barrier - wait for all memory writes

    // Write to Application Interrupt and Reset Control Register (AIRCR)
    SCB->AIRCR = (0x5FA << 16)  // Magic key (required for security)
               | (1 << 2);      // SYSRESETREQ bit - request reset

    __DSB();  // Ensure the write completes

    while(1);  // Wait for reset (we'll never get past this)
}
```

**Why the magic key (0x5FA)?**

This is a security feature.  You can't accidentally reset the system by writing garbage to this register – you must know the correct "password."

---

### 7.7 SYS__exit (Number: 3) - Terminating a Task

**Purpose**: End the current task and never return

**Arguments**: 

| Parameter | Type | Description |
|-----------|------|-------------|
| `status` | int | Exit code (not currently used, but kept for compatibility) |

**Returns**: Never returns

```c
static int32_t sys_exit_impl(int code)
{
    (void)code;  // We don't use the exit code, but suppress compiler warning

    // Step 1: Mark this task as terminated
    extern TCB_TypeDef* get_current_task(void);
    TCB_TypeDef *current = get_current_task();
    if (current != 0) {
        current->status = TASK_STATUS_KILLED;  // Mark as dead
    }

    // Step 2: Give up the CPU so another task can run
    task_yield();

    // Step 3: If we somehow get here (no other tasks), just wait forever
    while (1) {
        __WFI();  // Wait For Interrupt - low-power sleep
    }
    return 0;  // Never reached
}
```

#### What is __WFI()?

**Wait For Interrupt** is a low-power mode where the CPU stops executing until an interrupt occurs. It saves power and prevents the CPU from spinning uselessly.

```
TASK LIFECYCLE:
══════════════

      ┌────────────────┐
      │   Task Created │
      └───────┬────────┘
              │
              ▼
      ┌────────────────┐
      │     READY      │◄─────────────────────┐
      │  (waiting to   │                      │
      │     run)       │                      │
      └───────┬────────┘                      │
              │ scheduler                     │
              │ picks this task               │
              ▼                               │
      ┌────────────────┐      yield()        │
      │    RUNNING     │──────────────────────┘
      │  (executing)   │
      └───────┬────────┘
              │ _exit() called
              ▼
      ┌────────────────┐
      │     KILLED     │
      │  (terminated)  │
      │  (removed from │
      │   scheduler)   │
      └────────────────┘
```

---

## Chapter 8: Summary Table

| Syscall | ID | Purpose | Key Function | Arguments |
|---------|------|---------|--------------|-----------|
| `SYS_write` | 55 | Print to screen | `noIntWrite()` | fd, buffer, length |
| `SYS_read` | 50 | Read from keyboard | `Uart_read()` | fd, buffer, length |
| `SYS___time` | 113 | Get uptime in ms | `__getTime()` | none |
| `SYS_getpid` | 5 | Get task ID | `get_current_task()` | none |
| `SYS_yield` | 120 | Give up CPU | `task_yield()` → PendSV | none |
| `SYS_reboot` | 119 | Restart system | `NVIC_SystemReset()` | none |
| `SYS__exit` | 3 | Terminate task | Set KILLED + yield | exit_code |

---

## Chapter 9: Glossary

| Term | Definition |
|------|------------|
| **ARM Cortex-M4** | A type of CPU designed by ARM for embedded systems.  It's energy-efficient and commonly used in microcontrollers. |
| **Context Switch** | The process of saving one task's state and loading another task's state so the CPU can switch between tasks. |
| **EXC_RETURN** | A special value in the Link Register after an exception, used by hardware to know how to return to the previous state. |
| **Exception** | An event that causes the CPU to stop normal execution and jump to a special handler function. |
| **File Descriptor** | A number representing an open file or device.  0=stdin, 1=stdout, 2=stderr. |
| **Kernel** | The core of the operating system that manages hardware and provides services to programs. |
| **MSP (Main Stack Pointer)** | Stack pointer used by exception handlers and kernel code. |
| **NVIC** | Nested Vectored Interrupt Controller – the hardware that manages interrupts and exceptions. |
| **PendSV** | A low-priority exception used for context switching between tasks. |
| **Polling** | Repeatedly checking a condition (like "is data ready?") instead of waiting for an interrupt. |
| **Privileged Mode** | CPU mode with full access to hardware and memory. |
| **PSP (Process Stack Pointer)** | Stack pointer used by user tasks. |
| **Ring Buffer** | A circular data structure used for buffering, commonly used for UART receive data. |
| **Stack** | A memory region that grows/shrinks as functions are called/returned, storing local variables and return addresses. |
| **SVC (Supervisor Call)** | A CPU instruction that triggers an exception to request kernel services. |
| **Syscall** | System call – the interface between user programs and the operating system kernel. |
| **TCB (Task Control Block)** | Data structure containing all information about a task (ID, stack pointer, status, etc.). |
| **UART** | Universal Asynchronous Receiver-Transmitter – hardware for serial communication. |
| **Unprivileged Mode** | CPU mode with restricted access to hardware and memory. |
| **volatile** | C keyword indicating a variable may change unexpectedly (by hardware or interrupts). |

---

## Chapter 10: Quick Reference

### How to Make a System Call (User Code)

```c
#include <unistd.h>

int main(void)
{
    // Write "Hello" to the screen
    write(1, "Hello\n", 6);

    // Read a character from keyboard
    char c;
    read(0, &c, 1);

    // Get current task ID
    int pid = getpid();

    // Get system time in milliseconds
    uint32_t time = time_ms();

    // Voluntarily give up CPU
    yield();

    // Terminate this task
    _exit(0);

    // Reboot (use with caution!)
    reboot();

    return 0;
}
```

### Error Codes

| Error Code | Value | Meaning |
|------------|-------|---------|
| `EINVAL` | -22 | Invalid argument |
| `ENOSYS` | -38 | Function not implemented |

### Common File Descriptors

| Value | Name | Description |
|-------|------|-------------|
| 0 | `STDIN_FILENO` | Standard input (keyboard) |
| 1 | `STDOUT_FILENO` | Standard output (screen) |
| 2 | `STDERR_FILENO` | Standard error (screen) |

---

## Appendix A: Complete Exception Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           COMPLETE SYSCALL FLOW                             │
└─────────────────────────────────────────────────────────────────────────────┘

USER SPACE (Unprivileged)
═══════════════════════════════════════════════════════════════════════════════

  ┌──────────────────┐
  │  User Program    │
  │                  │
  │  write(1,        │
  │    "Hello", 5);  │
  └────────┬─────────┘
           │
           ▼
  ┌──────────────────┐
  │  unistd.c        │
  │                  │
  │  svc_invoke(     │
  │    SYS_write,    │
  │    1, buf, 5)    │
  └────────┬─────────┘
           │
           │  Registers loaded: 
           │  R0=55, R1=1, R2=buf, R3=5
           │
           ▼
  ┌──────────────────┐
  │  SVC #0          │ ◄─── Magic instruction that switches modes
  └────────┬─────────┘
           │
═══════════╪═══════════════════════════════════════════════════════════════════
           │                    MODE SWITCH BOUNDARY
═══════════╪═══════════════════════════════════════════════════════════════════
           │
           ▼

KERNEL SPACE (Privileged)
═══════════════════════════════════════════════════════════════════════════════

  ┌──────────────────────────────────────────┐
  │  HARDWARE: Auto-save to stack            │
  │                                          │
  │  Push:  R0, R1, R2, R3, R12, LR, PC, xPSR │
  └────────────────────┬─────────────────────┘
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │  SVCall_Handler (Assembly)               │
  │                                          │
  │  - Determine which stack (MSP or PSP)    │
  │  - Put stack pointer in R0               │
  │  - Branch to SVCall_Handler_C            │
  └────────────────────┬─────────────────────┘
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │  SVCall_Handler_C (C Code)               │
  │                                          │
  │  - Extract:  callno=55, a1=1, a2=buf, a3=5│
  │  - Call syscall_dispatch()               │
  │  - Put return value in stack_ptr[0]      │
  └────────────────────┬─────────────────────┘
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │  syscall_dispatch()                      │
  │                                          │
  │  switch(55) → sys_write_impl()           │
  └────────────────────┬─────────────────────┘
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │  sys_write_impl()                        │
  │                                          │
  │  - Check fd is stdout (1)                │
  │  - Loop through buffer                   │
  │  - Call noIntWrite() for each byte       │
  │  - Return 5 (bytes written)              │
  └────────────────────┬─────────────────────┘
                       │
                       │  Return value (5) written to stack_ptr[0]
                       │
                       ▼
  ┌──────────────────────────────────────────┐
  │  Exception Return (EXC_RETURN)           │
  │                                          │
  │  HARDWARE: Auto-restore from stack       │
  │  Pop: R0, R1, R2, R3, R12, LR, PC, xPSR  │
  │  (R0 now contains 5)                     │
  └────────────────────┬─────────────────────┘
                       │
═══════════════════════╪═══════════════════════════════════════════════════════
                       │                    MODE SWITCH BOUNDARY
═══════════════════════╪═══════════════════════════════════════════════════════
                       │
                       ▼

USER SPACE (Unprivileged)
═══════════════════════════════════════════════════════════════════════════════

  ┌──────────────────┐
  │  svc_invoke      │
  │  returns 5       │
  └────────┬─────────┘
           │
           ▼
  ┌──────────────────┐
  │  write() returns │
  │  5 to user       │
  │                  │
  │  Program         │
  │  continues...     │
  └──────────────────┘
```

---

## Appendix B: Testing Your Syscalls

DUOS includes an interactive testing menu.  Here's how to use it:

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

Global variables track syscall activity for debugging:

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

*This manual was created for the CSE-3211 Operating System Lab at the University of Dhaka to help students understand system call implementation on the STM32F446RE microcontroller running DUOS.*

*Document Version: 1.0*  
*Last Updated: January 2026*