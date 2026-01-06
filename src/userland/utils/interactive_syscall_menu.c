/*
 * Userland replica of the original kernel interactive_syscall_menu().
 * Runs unprivileged and uses only syscall wrappers (read/write/getpid/etc.).
 * Debug counters are accessed via extern declarations (defined in kernel).
 */

#include <unistd.h>
#include <stdint.h>
#include <times.h>
/* Local lightweight k_strlen implementation (avoid kernel dependency) */
static int k_strlen(const char *s) { int n=0; while(s[n]) n++; return n; }

/* Debug counters (defined in kernel source files) */
extern volatile uint32_t g_svcall_count;
extern volatile uint32_t g_last_callno;
extern volatile int32_t  g_last_retval;
extern volatile uint32_t g_syscall_dispatch_count;
extern volatile uint32_t g_last_syscall_id;
extern volatile uint32_t g_last_arg1;
extern volatile int32_t  g_last_syscall_ret;

static int read_line_u(char *buf, int max)
{
    int pos = 0;
    while (pos < max - 1) {
        /* Read up to one byte (sys_read blocks until at least one arrives) */
        char c = 0;
        int r = read(0, (uint8_t *)&c, 1);
        if (r < 0) return r; /* syscall error */
        if (r == 0) continue; /* still waiting */
        if (c == '\r' || c == '\n') { write(1, "\r\n", 2); break; }
        if (c == '\b' || c == 127) {
            if (pos > 0) { pos--; write(1, "\b \b", 3); }
            continue;
        }
        if (c >= 32 && c <= 126) { buf[pos++] = c; write(1, &c, 1); }
    }
    buf[pos] = '\0';
    return pos;
}

static int str_to_int_u(const char *s)
{
    int i = 0, sign = 1, v = 0;
    if (s[0] == '-') { sign = -1; i = 1; }
    while (s[i] >= '0' && s[i] <= '9') { v = v*10 + (s[i]-'0'); i++; }
    return sign * v;
}

/* write_str kept for potential future use */
static void write_str(const char *s) { write(1, s, k_strlen(s)); }

/* write a line and ensure CRLF line ending to keep terminals consistent */
static void write_line(const char *s)
{
    write(1, s, k_strlen(s));
    write(1, "\r\n", 2);
}

static void write_uint(const char *prefix, unsigned long v, const char *suffix)
{
    char tmp[32]; int ti=0;
    if (v==0) tmp[ti++]='0';
    while (v>0 && ti<(int)sizeof(tmp)-1){ tmp[ti++] = '0'+(v%10); v/=10; }
    write(1, prefix, k_strlen(prefix));
    char out[40]; int oi=0;
    for (int j=ti-1;j>=0;--j) out[oi++]=tmp[j];
    if (suffix){ int sl=k_strlen(suffix); for(int i=0;i<sl;i++) out[oi++]=suffix[i]; }
    write(1,out,oi);
}

void interactive_syscall_menu(void)
{
    char input[128];
    write_str("\n================================================\n");
    write_str("INTERACTIVE SYSCALL TESTING (userland)\n");
    write_str("================================================\n");
    write_str("All input uses read() syscall\n");
    write_str("All output uses write() syscall\n\n");

    /* Early raw key test to verify RX path before entering menu */
    write_str("Press any key to continue... ");
    char first = 0;
    int fr = read(0, &first, 1);
    if (fr > 0) {
        write_str("\nKey received: 0x");
        const char hexChars[] = "0123456789ABCDEF";
        char hx[3];
        hx[0] = hexChars[(first >> 4) & 0xF];
        hx[1] = hexChars[first & 0xF];
        hx[2] = '\0';
        write_str(hx);
        write_str(" ('");
        if (first >= 32 && first <= 126) write(1, &first, 1); else write_str(".");
        write_str("')\n\n");
    } else {
        write_str("\n(No key captured; read() returned ");
        char d = (char)('0' + fr); write(1,&d,1);
        write_str(")\n\n");
    }

    for (;;) {
    write_line("================================================");
    write_line("SYSCALL TESTING MENU");
    write_line("================================================");
    write_line("Select a syscall to test:");
    write_line("1) SYS_write   - Write to stdout/stderr");
    write_line("2) SYS_read    - Read from stdin");
    write_line("3) SYS___time  - Get system time");
    write_line("4) SYS_getpid  - Get process ID");
    write_line("5) SYS_yield   - Yield CPU to scheduler");
    write_line("6) SYS_reboot  - Reboot system (CAUTION!)");
    write_line("7) SYS__exit   - Exit and halt (CAUTION!)");
    write_line("8) Debug Info  - Show handler debug counters");
    write_line("9) Raw Key Monitor (diagnostic)");
    write_line("0) Exit        - Return to main");
    write_str("\nEnter choice (0-8): ");

    int len = read_line_u(input, sizeof(input));
    if (len == 0) { write_str("No input received. Try again.\n"); continue; }
    int choice = str_to_int_u(input);

        switch (choice) {
            case 1: { /* SYS_write */
                write_str("\n--- Testing SYS_write ---\nEnter message to write: ");
                len = read_line_u(input, sizeof(input));
                if (len > 0) {
                    write_str("\nCalling write(1, msg, len)...\n");
                    int written = write(1, input, len);
                    (void)written;
                    write_str("\nwrite() returned bytes.\n");
                } else {
                    write_str("No message entered.\n");
                }
                write_str("\nPress ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 2: { /* SYS_read */
                write_str("\n--- Testing SYS_read ---\nEnter number of bytes to read (1-50): ");
                read_line_u(input, sizeof(input));
                int bytes = str_to_int_u(input);
                if (bytes > 0 && bytes <= 50) {
                    char buf[64];
                    write_str("\nType your input: ");
                    int r = read(0, (uint8_t*)buf, bytes);
                    if (r > 0) {
                        buf[r] = '\0';
                        write_str("\nread() returned bytes: "); write(1, buf, r); write_str("\n");
                    } else if (r == 0) {
                        write_str("\nTimeout / no data.\n");
                    } else {
                        write_str("\nread() failed.\n");
                    }
                } else {
                    write_str("Invalid byte count!\n");
                }
                write_str("\nPress ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 3: { /* SYS___time */
                write_str("\n--- Testing SYS___time ---\n");
                uint32_t t1 = time_ms();
                write_uint("Current time: ", (unsigned long)t1, " ms\n");
                write_str("\nWaiting 1 second...\n");
                /* Busy wait ~1000ms using time_ms() loop if ms_delay unavailable */
                uint32_t start = time_ms();
                while ((uint32_t)(time_ms() - start) < 1000) { /* spin */ }
                uint32_t t2 = time_ms();
                write_uint("New time: ", (unsigned long)t2, " ms\n");
                unsigned long dt = (unsigned long)(t2 - t1);
                write_uint("Elapsed: ", dt, " ms (expected ~1000ms)\n");
                write_str("\nPress ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 4: { /* SYS_getpid */
                write_str("\n--- Testing SYS_getpid ---\nCalling getpid()...\n");
                int pid = getpid();
                write_uint("getpid() returned: ", (unsigned long)pid, "\n(Stub until threading)\n");
                write_str("Press ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 5: { /* SYS_yield */
                write_str("\n--- Testing SYS_yield ---\nCalling yield()...\n");
                int yret = yield(); (void)yret;
                write_str("yield() invoked (PendSV requested)\n");
                write_str("Press ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 6: { /* SYS_reboot */
                write_str("\n--- Testing SYS_reboot ---\nWARNING: This will RESET the board!\nType 'YES' to confirm: ");
                read_line_u(input, sizeof(input));
                if (input[0]=='Y'&&input[1]=='E'&&input[2]=='S'&&input[3]=='\0') {
                    write_str("\nRebooting in 2 seconds...\n");
                    uint32_t st = time_ms();
                    while ((uint32_t)(time_ms() - st) < 2000) { /* spin */ }
                    reboot();
                } else write(1, "Reboot cancelled.\n", 19);
                break;
            }
            case 7: { /* SYS__exit */
                write_str("\n--- Testing SYS__exit ---\nWARNING: This will HALT the system!\nType 'YES' to confirm: ");
                read_line_u(input, sizeof(input));
                if (input[0]=='Y'&&input[1]=='E'&&input[2]=='S'&&input[3]=='\0') {
                    write_str("\nExiting...\n"); _exit(0);
                } else write(1, "Exit cancelled.\n", 16);
                break;
            }
            case 8: { /* Debug counters */
                write_str("\n--- SVCall Handler Debug Info ---\n");
                write_uint("  Handler called: ", g_svcall_count, " times\n");
                write_uint("  Last syscall number: ", g_last_callno, "\n");
                write_uint("  Last return value: ", (unsigned long)g_last_retval, "\n");
                write_str("\n--- Syscall Dispatcher Debug Info ---\n");
                write_uint("  Dispatch called: ", g_syscall_dispatch_count, " times\n");
                write_uint("  Last syscall id: ", g_last_syscall_id, "\n");
                write_uint("  Last arg1: ", g_last_arg1, "\n");
                write_uint("  Last retval: ", (unsigned long)g_last_syscall_ret, "\n");
                write_str("\nPress ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
            }
            case 9: { /* Raw key monitor */
                write_str("\n--- Raw Key Monitor ---\nPress keys; ESC to return to menu.\n");
                for (;;) {
                    unsigned char ch=0; int r = read(0,&ch,1);
                    if (r < 0) { write_str("\nread error\n"); break; }
                    if (r == 0) continue; /* waiting */
                    if (ch == 27) { write_str("\n(ESC) Exit raw monitor\n"); break; }
                    write_str("Char: 0x");
                    const char hex[] = "0123456789ABCDEF";
                    char hx[3]; hx[0]=hex[(ch>>4)&0xF]; hx[1]=hex[ch&0xF]; hx[2]='\0'; write_str(hx);
                    write_str("  '"); if (ch>=32 && ch<=126) write(1,(char*)&ch,1); else write_str("."); write_str("'\n");
                }
                write_str("Press ENTER to continue..."); read_line_u(input,sizeof(input));
                break;
            }
            case 0:
                write_str("\nExiting interactive menu...\n");
                return;
            default:
                write_str("\nInvalid choice! Please select 0-8.\n");
                write_str("Press ENTER to continue..."); read_line_u(input, sizeof(input));
                break;
        }
    }
}
