#include "u-lib.hh"

// dprintf
//    Construct a string from `format` and pass it to `sys_write(fd)`.
//    Returns the number of characters printed, or E_2BIG if the string
//    could not be constructed.

int dprintf(int fd, const char* format, ...) {
    char buf[513];
    va_list val;
    va_start(val, format);
    size_t n = vsnprintf(buf, sizeof(buf), format, val);
    if (n < sizeof(buf)) {
        return sys_write(fd, buf, n);
    } else {
        return E_2BIG;
    }
}


// printf
//    Like `dprintf(1, ...)`.

int printf(const char* format, ...) {
    char buf[513];
    va_list val;
    va_start(val, format);
    size_t n = vsnprintf(buf, sizeof(buf), format, val);
    if (n < sizeof(buf)) {
        return sys_write(1, buf, n);
    } else {
        return E_2BIG;
    }
}


// panic, assert_fail
//     Call the SYSCALL_PANIC system call so the kernel loops until Control-C.

void panic(const char* format, ...) {
    va_list val;
    va_start(val, format);
    char buf[160];
    memcpy(buf, "PANIC: ", 7);
    int len = vsnprintf(&buf[7], sizeof(buf) - 7, format, val) + 7;
    va_end(val);
    if (len > 0 && buf[len - 1] != '\n') {
        strcpy(buf + len - (len == (int) sizeof(buf) - 1), "\n");
    }
    error_printf(CS_ERROR "%s", buf);
    sys_panic(nullptr);
}

void error_vprintf(const char* format, va_list val) {
    int scroll_mode = console_printer::scroll_on;
    if (consoletype != CONSOLE_NORMAL) {
        scroll_mode = console_printer::scroll_blank;
    }
    console_printer pr(-1, scroll_mode);
    if (consoletype != CONSOLE_NORMAL
        && pr.cell_ < console + END_CPOS - CONSOLE_COLUMNS) {
        pr.cell_ = console + END_CPOS;
    }
    pr.vprintf(format, val);
    pr.move_cursor();
}

void assert_fail(const char* file, int line, const char* msg,
                 const char* description) {
    if (description) {
        error_printf("%s:%d: %s\n", file, line, description);
    }
    error_printf("%s:%d: user assertion '%s' failed\n", file, line, msg);
    sys_panic(nullptr);
}


// sys_clone
//    Create a new thread.

pid_t sys_clone(int (*function)(void*), void* arg, char* stack_top) {
    // Your code here
    return E_NOSYS;
}
