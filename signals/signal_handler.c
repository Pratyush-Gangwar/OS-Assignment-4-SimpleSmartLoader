#include "../headers.h"

#define PAGE_SIZE 4096

int num_pages_allocated = 0;
int num_page_faults = 0;
int fragmentation = 0;

// from loader.c
extern Elf32_Ehdr *ehdr;
extern Elf32_Phdr *phdr;
extern int fd;

int min(int a, int b) {
    return ( a <= b ? a : b );
}

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

void* round_down(void* fault_addr) {
    int int_addr = (int) fault_addr;
    int rounded_down_int_addr = (int_addr/PAGE_SIZE) * PAGE_SIZE; // division operator performs floor-division
    return (void*) rounded_down_int_addr;
}

void handler(int sig, siginfo_t* info, void* ucontext) {
    num_page_faults++; // SIGSEGV indicates invalid memory access. May be due to page fault or due to actual invalid memory access (ex, accessing out of bounds array index)

    void* fault_addr = info->si_addr; // address that caused SIGSEGV
    int idx = get_segment_idx(fault_addr);

    if (idx == -1) { // actual invalid access; not page fault
        return; // don't allocate page
    }

    void* page_start = round_down(fault_addr); // closest page boundary
    void* mmap_addr = mmap(page_start, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);

    if (mmap_addr == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    if (lseek(fd, phdr[idx].p_offset, SEEK_SET) == -1) { // seek to beginning of segment in file
        perror("Error while seeking to the executable segment");
        exit(1);
    }

    // vaddr + filesz - 1 is the address of last byte in the segment read from disk
    // total bytes are (address of last byte from disk) - (page start) + 1
    // if memsz > filesz, then last byte in segment is vaddr + memsz - 1 but last byte read from disk is still vaddr + filesz - 1

    // Case 1: filesz <= PAGE_SIZE. Only one page is required. 4KB has already been allocated by mmap. 
    // We read vaddr + filesz - page_start bytes from the file

    // Case 2: filesz > PAGE_SIZE. Two or more pages are required. One 4KB page has already been allocated by mmap.
    // If N pages are required (N = ceil(filesz/PAGE_SIZE)), then first N-1 pages in memory are fully filled from the bytes on the disk
    // the last page may or may not be fully filled by bytes from disk

    // Case 2.1: Faulty address arose from 1 ... N - 1 page. Full PAGE_SIZE read from disk
    // Case 2.2: Faulty address arose from N page. min( vaddr + filesz - page_start, PAGE_SIZE) bytes read from the file

    int bytes = min(phdr[idx].p_vaddr + phdr[idx].p_filesz - (int) page_start, PAGE_SIZE);
    if (read(fd, mmap_addr, PAGE_SIZE) == -1) { // read segment bytes from file to memory
        perror("Error while writing bytes from file to virtual memory");
        exit(1);
    }

    fragmentation += PAGE_SIZE - bytes;
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