#include "../loader/loader.h"

int num_pages_allocated = 0;
int num_page_faults = 0;
int fragmentation = 0;

// from loader.c
extern Elf32_Ehdr *ehdr;
extern Elf32_Phdr *phdr;

int get_segment_idx(void* fault_addr) {
    for(int i = 0; i < ehdr->e_phnum; i++) {    
                
        // fault_adrr must lie in range [p_vaddr, p_vaddr + p_memsz)
        // attributes of phdr are actually ints. we must typecast them to void*
        if ((void*) phdr[i].p_vaddr <= fault_addr && fault_addr < (void*) (phdr[i].p_vaddr + phdr[i].p_memsz) ){
            return i;
        } 
  } 

  return -1;

}

void handler(int sig, siginfo_t* info, void* ucontext) {
    num_page_faults++; // SIGSEGV indicates invalid memory access. May be due to page fault or due to actual invalid memory access (ex, accessing out of bounds array index)

    void* fault_addr = info->si_addr; // address that caused SIGSEGV
    int idx = get_segment_idx(fault_addr);

    if (idx == -1) { // actual invalid access; not page fault
        return; // don't allocate page
    }

    num_pages_allocated++;
}

void setup_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    // sa_sigaction lets us find fault address via siginfo_t struct
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;

    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(1);
    }
}