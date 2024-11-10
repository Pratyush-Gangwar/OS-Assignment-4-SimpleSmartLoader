#include "headers.h"

extern int num_pages_allocated;
extern int num_page_faults;
extern int fragmentation_theoretical;
extern int fragmentation_practical;
extern int fragmentation_bytes_read;

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  setup_signal_handlers();

  // 1. carry out necessary checks on the input ELF file, 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);

  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();

  printf("Number of pages allocated: %d\n", num_pages_allocated);
  printf("Number of page faults: %d\n", num_page_faults);

  printf("Internal (theoretical) fragmentation: %d bytes = %f KB \n", fragmentation_theoretical, ( (float) fragmentation_theoretical )/1024);
  printf("Internal (practical) fragmentation: %d bytes = %f KB \n", fragmentation_practical, ( (float) fragmentation_practical )/1024);
  printf("Internal (bytes read) fragmentation: %d bytes = %f KB\n", fragmentation_bytes_read, ( (float) fragmentation_bytes_read )/1024);

  return 0;
}
