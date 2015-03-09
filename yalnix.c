#include <comp421/hardware.h>
#include <comp421/yalnix.h>


int 
SetKernelBrk(void *addr) {
	if(virt_mem_initialized){
		//more complicated stuff
		} else {
			kernel_brk = addr;
			// modify the structure accordingly that keeps track of free physical page frames
		}
	}
}

void 
KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args){

}