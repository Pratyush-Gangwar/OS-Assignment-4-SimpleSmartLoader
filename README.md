# Group 121 (Solo)
Pratyush Gangwar

# Github
https://github.com/Pratyush-Gangwar/OS-Assignment-4-SimpleSmartLoader

# Working
- SIGSEGV signal is caught 
- sigaction.sa_sigaction is used to define the handler function instead of sigaction.sa_handler
- this is because sa_sigaction passes a struct to the function which contains information about the SIGNAL such as the address at which SEGFAULT occured

- siginfo_t->addr is used to get the address where SEGFAULT occurs
- the nearest page-boundary is calculated by rounding down this address
- this rounding down is done by integer-dividing the fault address by PAGE_SIZE and then mutltiplying the result by PAGE_SIZE

- afterwards, the segment in which this fault address lies in is found
- done by iterating over all the program headers in the ELF file (these were stored in an array created by loader.c)
- the following condition is checked (void*) phdr[i].p_vaddr <= fault_addr && fault_addr < (void*) (phdr[i].p_vaddr + phdr[i].p_memsz)

- an mmap of PAGE_SIZE is done at the page boundary

- the offset of the page boundary from the virtual address of the first byte of the segment is calculated 
- if this offset is less than filesz, then a seek to p_offset + offset is permissible
- otherwise, such a seek is invalid as it would lead you to read bytes of the next segment in the file

- in case the seek is valid, two cases arise. either filesz <= PAGE_SIZE or filesz > PAGE_SIZE
    - filesz <= PAGE_SIZE
        - filesz bytes are read from p_offset + offset into memory 
        - if memsz > filesz, then the remaining bytes are memset to 0
        - fragmentation is PAGE_SIZE - memsz

    - filesz > PAGE_SIZE
        - the bytes in the file for this segment can be divided into "internal" pages and the last page
        - if the segfault occured in an internal page, then PAGE_SIZE bytes are read from the file into memory and fragmentation is zero

        - if segfault occured in the last page, then filesz - offset btyes are read from file into memory and fragmentation is PAGE_SIZE - (memsz - offset)
        - memsz - offset represents the portion of memsz inside the last page
        - if memsz > filesz, remaining bytes are cleared to zero


- in case the seek is invalid, two cases arise. either the segfault occured in an internal page or the last page
    - in both cases, zero bytes are read from file since the seek is invalid
    - if the segfault occured in an internal page, then fragmentation is zero
    - if the segfault occured in the last page, then fragmentation is PAGE_SIZE - (memsz - offset)
    - if memsz > filesz, remaining bytes are cleared to zero