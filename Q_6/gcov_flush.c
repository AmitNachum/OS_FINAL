// gcovflush.c - force gcov to flush on exit and on SIGINT/SIGTERM
#define _GNU_SOURCE
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void __gcov_flush(void) __attribute__((weak)); 
static void flush_now(void) {
    if (__gcov_flush) __gcov_flush();
}

static void handler(int sig) {
    flush_now();
    // restore default and re-raise so the program terminates with the same signal
    struct sigaction sa = {0};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
    kill(getpid(), sig);
}

__attribute__((constructor))
static void setup(void) {
    // flush on normal process exit
    atexit(flush_now);

    // also flush on Ctrl-C and SIGTERM
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
