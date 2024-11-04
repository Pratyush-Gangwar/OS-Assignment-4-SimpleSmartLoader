#include "../loader/loader.h"

void handler(int sig, siginfo_t* info, void* ucontext) {

}

void setup_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    // sa_sigaction tells us find fault address via siginfo_t struct
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }
}