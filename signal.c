#include "signal.h"
#include "process.h"
#include "vga.h"

static signal_handler_t handlers[64];

void signal_init(void) {
    uint32_t i;
    for (i = 0; i < 64; i++) handlers[i] = NULL;
}

int signal_send(uint32_t pid, int sig) {
    if (sig < 0 || sig > 63) return -1;
    process_t* p = process_get(pid);
    if (!p) return -1;

    if (sig == SIGKILL) {
        p->state = PROC_TERMINATED;
        return 0;
    }
    if (sig == SIGSTOP) {
        p->state = PROC_BLOCKED;
        p->wake_tick = 0;
        return 0;
    }
    if (sig == SIGCONT) {
        if (p->state == PROC_BLOCKED) p->state = PROC_READY;
        return 0;
    }
    if (handlers[sig]) {
        handlers[sig](sig);
    } else {
        signal_default_action(sig);
    }
    return 0;
}

void signal_handle(int sig) {
    if (sig == SIGINT || sig == SIGQUIT) {
        vga_puts("\n^C\n");
    } else if (sig == SIGTERM) {
        process_t* p = process_get_current();
        if (p) p->state = PROC_TERMINATED;
    }
}

void signal_default_action(int sig) {
    if (sig == SIGKILL || sig == SIGTERM || sig == SIGSEGV || sig == SIGABRT) {
        process_t* p = process_get_current();
        if (p) p->state = PROC_TERMINATED;
    }
}

const char* signal_name(int sig) {
    switch (sig) {
        case SIGNULL:  return "NULL";
        case SIGHUP:   return "HUP";
        case SIGINT:    return "INT";
        case SIGQUIT:   return "QUIT";
        case SIGILL:    return "ILL";
        case SIGTRAP:   return "TRAP";
        case SIGABRT:   return "ABRT";
        case SIGKILL:   return "KILL";
        case SIGUSR1:   return "USR1";
        case SIGSEGV:   return "SEGV";
        case SIGUSR2:   return "USR2";
        case SIGALRM:   return "ALRM";
        case SIGTERM:   return "TERM";
        case SIGCHLD:   return "CHLD";
        case SIGSTOP:   return "STOP";
        case SIGCONT:   return "CONT";
        default:        return "???";
    }
}
