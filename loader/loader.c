#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
char* virtual_mem;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  // close the file
  close(fd);

  // free the malloc'ed memory
  free(ehdr);
  free(phdr);
}

/*
 * Load and run the ELF executable file
 */

void load_and_validate(char** exe) {
  // Open file
  fd = open(exe[1], O_RDONLY);

  // open() returns -1 on error
  if (fd == -1) {
    perror("Error while opening file");
    exit(1);
  }

  // Load first 5 bytes - first four are ELF 'magic' and fifth is ELFCLASS
  unsigned char e_ident[5];

  // read() returns -1 on error
  if (read(fd, e_ident, 5) == -1) {
    perror("Error while reading\n");
    exit(1);
  }
  
  if ( !(e_ident[0] == 0x7f && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F') ) {
    printf("Not an ELF file\n");
    exit(1);
  }

  // ELFCLASS is 1 for 32-bit, 2 for 64-bit
  if ( !(e_ident[4] == 1) ) {
    printf("Not 32-bit ELF fle\n");
    exit(1);
  }

  // lseek() returns (off_t) - 1 on error
  if (lseek(fd, 0, SEEK_SET) == (off_t) - 1) {   // Seek back to the start of the file so that ELF header can be properly loaded in load_and_run_elf()
    perror("Error while seeking to start:");
    exit(1);
  }
}

void load_and_run_elf(char** exe) {

  // Validate file
  load_and_validate(exe);
  
  // 1. Load entire binary content into the memory from the ELF file.
  ehdr = malloc( sizeof(Elf32_Ehdr) ); // allocate space for ELF header

  if (ehdr == NULL) { // malloc returns NULL on error
    perror("Error while malloc'ing Ehdr:");
    exit(1);
  }

  if (read(fd, ehdr, sizeof(Elf32_Ehdr)) == -1) { // read ELF header bytes from file to memory
    perror("Error while reading Ehdr:");
    exit(1);
  }

  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c

  if (lseek(fd, ehdr->e_phoff, SEEK_SET) == (off_t) - 1) { // go to beginning of program header table
    perror("Error while seeking to program header table:");
    exit(1);
  }

  int entry_found = 0; // flag to check if entry point was found

  phdr = malloc( ehdr->e_phnum * sizeof(Elf32_Phdr) ); // allocate memory for program header array

  if (phdr == NULL) { 
    perror("Error while malloc'ing Phdr");
    exit(1);
  }

  for(int i = 0; i < ehdr->e_phnum; i++) {
    if (read(fd, &phdr[i], sizeof(Elf32_Phdr)) == -1) { // read program header bytes from file to memory
      perror("Error while reading Phdr");
      exit(1);
    }
    
    // segment must be of type PT_LOAD and entry_point must lie in range [p_vaddr, p_vaddr + p_memsz)
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_vaddr <= ehdr->e_entry && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
      entry_found = 1; 
    } 
  } 

  if (!entry_found) {
    printf("Entry point doesn't exist");
    exit(1);
  }

  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  int (*_start)() = ( int(*)() ) ehdr->e_entry;

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);
}