void
occupy_kernel_pages_up_to(void *end) {
  if(is_page_occupied != NULL){
    occupy_pages_in_range(kernel_brk, end);
  }
  kernel_brk = (void *)UP_TO_PAGE(end);
}

void
occupy_pages_in_range(void *begin, void *end) {
  int i;
  int boundary = (long)UP_TO_PAGE(end)/PAGESIZE;
  int start = (long)DOWN_TO_PAGE(begin)/PAGESIZE;

  for(i = start; i < boundary; i++){
    is_page_occupied[i] = 1;
  }
}