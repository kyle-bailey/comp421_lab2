#include "load_program.h"

#include "memory_management.h"
#include "page_table_management.h"
#include "process_scheduling.h"
#include "process_control_block.h"

/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
int
LoadProgram(char *name, char **args, ExceptionStackFrame *frame, struct pte *page_table_to_load)
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char **cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int data_bss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    if ((fd = open(name, O_RDONLY)) < 0) {
      TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
      return (-1);
    }

    status = LoadInfo(fd, &li);
    TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
    switch (status) {
    case LI_SUCCESS:
        break;
    case LI_FORMAT_ERROR:
        TracePrintf(0,
        "LoadProgram: '%s' not in Yalnix format\n", name);
        close(fd);
        return (-1);
    case LI_OTHER_ERROR:
        TracePrintf(0, "LoadProgram: '%s' other error\n", name);
        close(fd);
        return (-1);
    default:
        TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
        close(fd);
        return (-1);
    }
    TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
    li.text_size, li.data_size, li.bss_size);

    TracePrintf(0, "entry 0x%lx\n", li.entry);


    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
      size += strlen(args[i]) + 1;
    }
    argcount = i;
    TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);


    /*
     *  Now save the arguments in a separate buffer in Region 1, since
     *  we are about to delete all of Region 0.
     */
    cp = argbuf = (char *)malloc(size);
    for (i = 0; args[i] != NULL; i++) {
      strcpy(cp, args[i]);
      cp += strlen(cp) + 1;
    }
  
    /*
     *  The arguments will get copied starting at "cp" as set below,
     *  and the argv pointers to the arguments (and the argc value)
     *  will get built starting at "cpp" as set below.  The value for
     *  "cpp" is computed by subtracting off space for the number of
     *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
     *  terminate the auxiliary vector, a NULL pointer terminating
     *  the argv pointers, and a NULL pointer terminating the envp
     *  pointers) times the size of each (sizeof(void *)).  The
     *  value must also be aligned down to a multiple of 8 boundary.
     */
    cp = ((char *)USER_STACK_LIMIT) - size;
    cpp = (char **)((unsigned long)cp & (-1 << 4));    /* align cpp */
    cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

    text_npg = li.text_size >> PAGESHIFT;
    data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n",
    text_npg, data_bss_npg, stack_npg);


    /*
     *  Make sure we will leave at least one page between heap and stack
     */
    if (MEM_INVALID_PAGES + text_npg + data_bss_npg + stack_npg + 1 + KERNEL_STACK_PAGES >= PAGE_TABLE_LEN) {
      TracePrintf(0, "LoadProgram: program '%s' size too large for VM\n",
          name);
      free(argbuf);
      close(fd);
      return (-1);
    }

    /*
     *  And make sure there will be enough physical memory to
     *  load the new program.
     *  The new program will require text_npg pages of text,
     *  data_bss_npg pages of data/bss, and stack_npg pages of
     *  stack. Additionally, allow for the physical memory
     *  allocated to this process that will be freed.
     */

    int required_free_physical_pages = text_npg + data_bss_npg + stack_npg - num_pages_in_use(page_table_to_load);

    if (num_free_physical_pages() < required_free_physical_pages) {
      TracePrintf(0, "LoadProgram: program '%s' size too large for physical memory\n", name);
      free(argbuf);
      close(fd);
      return (-1);
    }

    //Initialize sp for the current process to (char *)cpp.
    frame->sp = (char *)cpp;

    TracePrintf(3, "LoadProgram: Stack Pointer Initialized.\n");


    /*
     *  Free all the old physical memory belonging to this process,
     *  but be sure to leave the kernel stack for this process (which
     *  is also in Region 0) alone.
     */
    for(i = 0; i < PAGE_TABLE_LEN - KERNEL_STACK_PAGES; i++) {
      if(page_table_to_load[i].valid == 1){
        //free physical memory and set as invalid
        free_physical_page(page_table_to_load[i].pfn);
        page_table_to_load[i].valid = 0;
      }
    }

    TracePrintf(3, "LoadProgram: old physical memory belonging to process freed.\n");


    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    //Leave the first MEM_INVALID_PAGES number of PTEs unused
    int text_data_bss_top_boundary = text_npg + data_bss_npg;
    for(i = 0; i < text_data_bss_top_boundary; i++){
      if(i < text_npg){
        page_table_to_load[i + MEM_INVALID_PAGES].valid = 1;
        page_table_to_load[i + MEM_INVALID_PAGES].kprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i + MEM_INVALID_PAGES].uprot = PROT_READ | PROT_EXEC;
        page_table_to_load[i + MEM_INVALID_PAGES].pfn = acquire_free_physical_page();
      } else if( i < data_bss_npg + text_npg){
        page_table_to_load[i + MEM_INVALID_PAGES].valid = 1;
        page_table_to_load[i + MEM_INVALID_PAGES].kprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i + MEM_INVALID_PAGES].uprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i + MEM_INVALID_PAGES].pfn = acquire_free_physical_page();
      }
    }

    //Set the brk for the current process
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    pcb->brk = (void *)UP_TO_PAGE((MEM_INVALID_PAGES + text_npg + data_bss_npg) * PAGESIZE);

    TracePrintf(3, "LoadProgram: Text and data&bss prepped.\n");

    //the user stack grows downwards from just below the kernel stack.
    //the last page of the user stack *ends* at virtual address USER_STACK_LIMIT
    int last_user_page = USER_STACK_LIMIT/PAGESIZE - 1;
    for (i = last_user_page; i > last_user_page - stack_npg; i--) {
        page_table_to_load[i].valid = 1;
        page_table_to_load[i].kprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i].uprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i].pfn = acquire_free_physical_page();
    }

    //initialize the user_stack_limit
    pcb->user_stack_limit = (void *)DOWN_TO_PAGE(USER_STACK_LIMIT);

    TracePrintf(3, "LoadProgram: User stack prepped.\n");

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    TracePrintf(3, "LoadProgram: Just finished flushing TLB.\n");

    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size) != li.text_size+li.data_size) {
      TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
      free(argbuf);
      close(fd);
      return (-2);
    }

    close(fd);            /* we've read it all now */

    TracePrintf(3, "LoadProgram: Dont reading text and data from file into memory.\n");

    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */
    for(i = MEM_INVALID_PAGES; i < text_npg + MEM_INVALID_PAGES; i++){
      page_table_to_load[i].kprot = PROT_READ | PROT_EXEC;
    }

    TracePrintf(3, "LoadProgram: PTEs for program text modified.\n");

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
    '\0', li.bss_size);

    /*
     *  Set the entry point in the exception frame.
     */
    frame->pc = (void *)li.entry;

    TracePrintf(3, "LoadProgram: Program Counter set.\n");

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;        /* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
      *cpp++ = cp;
      strcpy(cp, cp2);
      cp += strlen(cp) + 1;
      cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;    /* the last argv is a NULL pointer */
    *cpp++ = NULL;    /* a NULL pointer for an empty envp */
    *cpp++ = 0;        /* and terminate the auxiliary vector */

    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     */
    for (i=0; i < NUM_REGS; i++) {
        frame->regs[i] = 0;
    }
    frame->psr = 0;

    TracePrintf(1, "LoadProgram: LoadProgram completed.\n");

    return (0);
}
