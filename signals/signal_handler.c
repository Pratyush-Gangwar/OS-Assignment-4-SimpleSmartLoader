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

int get_phdr_idx(void* fault_addr) {
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
    int phdr_idx = get_phdr_idx(fault_addr); // index of segment that the segfault address lies in

    if (phdr_idx == -1) { // actual invalid access; not page fault
        return; // don't allocate page
    }

    void* page_start = round_down(fault_addr); // closest page boundary

    // first argument is not NULL. So, OS will try to allocate memory at the specified address (page_start)
    // if a mapping already exists at that address, MAP_FIXED will cause the pre-existing mapping to be overwritten

    // since each process has its own virtual address space, we only have to worry about mmap() overwriting the SimpleLoader's process image 
    // we do not have to worry about it overwriting the process image for any other process
    void* mmap_addr = mmap(page_start, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, 0, 0); 

    if (mmap_addr == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    int offset = (char*) page_start - (char*) phdr[phdr_idx].p_vaddr; // offset of page boundary from start of segment
    // int prev_frag = fragmentation;
    // printf("phdr idx: %d memsz: %d vaddr: %d ", phdr_idx, phdr[phdr_idx].p_memsz, phdr[phdr_idx].p_vaddr);

    
    if (offset < phdr[phdr_idx].p_filesz) { // happens when filesz == memsz
        int bytes = 0;

        // we won't seek to start of segment in file
        // rather, we seek to the byte in the file on disk corresponding to virtual address where the segfault occured
        if (lseek(fd, phdr[phdr_idx].p_offset + offset, SEEK_SET) == -1) {
            perror("Error while seeking to the executable segment");
            exit(1);
        }

        // Case 1: filesz <= PAGE_SIZE. Only one page is required. 4KB has already been allocated by mmap. 
        if (phdr[phdr_idx].p_filesz <= PAGE_SIZE) {

            // we read filesz bytes from the file
            // and set the remaining bytes to zero 
            bytes = phdr[phdr_idx].p_filesz;
            memset(page_start + bytes, 0, phdr[phdr_idx].p_memsz - phdr[phdr_idx].p_filesz);

            // fragmentation calculated from memory size, not bytes read
            fragmentation += PAGE_SIZE - phdr[phdr_idx].p_memsz; 
        }

        else {

            // Case 2: filesz > PAGE_SIZE. Two or more pages are required. One 4KB page has already been allocated by mmap.
            // If N pages are required (N = ceil(filesz/PAGE_SIZE)), then first N-1 pages in memory are fully filled from the bytes on the disk
            // the last page may or may not be fully filled by bytes from disk
        
            // Case 2.1: Faulty address arose from 0 ... N - 2 page. Full PAGE_SIZE read from disk.
            // offset = 0, PAGE_SIZE, ..., (N - 2) * PAGE_SIZE
            // offset = (idx) * PAGE_SIZE

            int page_idx = offset/PAGE_SIZE;
            int N = phdr[phdr_idx].p_filesz/PAGE_SIZE + 1;

            if (0 <= page_idx && page_idx < N - 1) {
                bytes = PAGE_SIZE;
                fragmentation += 0; // since we read an entire page, there's no fragementation
            }

            // Case 2.2: Faulty address arose from N - 1 page. filesz - offset bytes read from the file
            else if (page_idx == N - 1) {

                bytes = phdr[phdr_idx].p_filesz - offset;
                memset(page_start + bytes, 0, phdr[phdr_idx].p_memsz - phdr[phdr_idx].p_filesz);

                // Previous 0...N-2 pages occupy (N - 1) * PAGE_SIZE
                // Memory occupied by this segment is memsz - memory occupied by previous pages     
                fragmentation += PAGE_SIZE - (phdr[phdr_idx].p_memsz - (N - 1) * PAGE_SIZE);
            }

        }

        if (read(fd, page_start, bytes) == -1) { // read segment bytes from file to memory
            perror("Error while writing bytes from file to virtual memory");
            exit(1);
        }
    
    } else { // filesz < memsz

        // printf("filesz < memsz ");
        int page_idx = offset/PAGE_SIZE;
        int N = phdr[phdr_idx].p_memsz/PAGE_SIZE + 1; // calculate N via memsz, not filesz

        // printf("page idx: %d N: %d ", page_idx, N);

        if (0 <= page_idx && page_idx < N - 1) {
            // printf("(internal page) ");
            fragmentation += 0; // since we read an entire page, there's no fragementation
        }

        // Case 2.2: Faulty address arose from N - 1 page. filesz - offset bytes read from the file
        else if (page_idx == N - 1) {
            fragmentation += PAGE_SIZE - (phdr[phdr_idx].p_memsz - (N - 1) * PAGE_SIZE);
        }

        // no read, just clear the bytes
        memset(page_start, 0, PAGE_SIZE);
    }

    num_pages_allocated++;

    // printf("Seg fault at %p Page boundary: %p Offset: %d Bytes read: %d Fragmentation Delta: %d \n", fault_addr, page_start, offset, bytes, fragmentation - prev_frag);
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