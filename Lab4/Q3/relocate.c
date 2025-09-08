#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<sys/mman.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

extern char etext;
extern long _start;
extern char end;

static unsigned call_f2(unsigned long addr, unsigned char xhash)
{
   int i = 0;
   unsigned result = (addr >> 32);
   result &= (addr & 0xFFFFFFFF);  
   return result;
}

static unsigned call_f1(unsigned long addr)
{
   int i = 0;
   unsigned char xhash = 0;	
   char buf[64];	
   sprintf(buf, "%lx", addr);
   while(buf[i]){
	 xhash = xhash ^ buf[i];
         ++i;	 
   }   
   return call_f2(addr, xhash);
}

/*XXX Do not change anything above this line*/

void *relocate_textseg(unsigned long start, unsigned long end)
{
      void *reloc_addr = NULL;	
     /*TODO your code goes in here*/

      unsigned long len = end - start;
      // len = ((len >> 12) << 12) + 4096;
      reloc_addr = mmap(NULL,len,PROT_WRITE|PROT_EXEC|PROT_READ,MAP_ANONYMOUS|MAP_PRIVATE,0,0);

     // void* ptr2 = memcpy(reloc_addr,(void*)(start),len);
     for(unsigned long i = 0 ; i < len ; i+=4096){
	     // so i is going page by page in the len which is from start to end
	     void* ptr3 = mmap((void*)(start+i),4096,PROT_WRITE|PROT_READ,MAP_ANONYMOUS|MAP_FIXED_NOREPLACE|MAP_PRIVATE,0,0);
	     // try to allocate the page to test if it's occupied or not
	     if(ptr3 == (void*)(-1)){
		     //copy the entire page
		     for(unsigned long j = 0 ; j < 4096 && i+j < len ; j ++){
			     *((char*)(reloc_addr) + i + j) = *(char *)(start+i + j);
		     }
	     }
	     else{
		     //free the page
		     munmap(ptr3,4096);
		     munmap((void*)((unsigned long)(reloc_addr) + i),4096);
	     }
     }

     return reloc_addr;   
}

int main(int argc, char **argv)
{

   unsigned long start_code = (unsigned long)&_start;   //Entry point of the binary
   unsigned long end_code = (unsigned long) &etext;
   unsigned long end_data = (unsigned long) &end;
   start_code = (start_code >> 12) << 12;   
   
   //Putting it at page boundary. NOTE: there may be some useful stuff before '_init'

   /*argc is called with different values for first and second invocation of main*/
   if(argc)
	   printf("##### First Call to main #####\n");
   else
	   printf("##### Repeat Call to main #####\n");
   
   if(argc){ 
   
	 void *reloc_main = NULL;  
         void *reloc_code_addr = NULL;
	 /* XXX Do not change anything above this line  XXX */
	  
	 /* TODO  You need to fill the reloc_code_addr with a properly mapped
	   *      code segment (relocated) that contains the main
	   *      function (and other functions). You need to also set 
	   *      the corresponding address of main (reloc_main) in the 
	   *      relocated executable area. Invoke the `relocate_textseg'
	   *      function with proper arguments and do the required processing
	   *      such that both values are correct.
	  */ 
	 reloc_code_addr = relocate_textseg(start_code,end_data);
	 unsigned long diff = (unsigned long)(&main)-start_code;
	 reloc_main = (void*)(diff +(unsigned long)reloc_code_addr);




	 
	 /* XXX Do not change anything below XXX */

	 printf("Relocated Code Segment:%lu:%lx\n", (unsigned long)reloc_code_addr, (unsigned long)reloc_code_addr);
	 printf("Relocated Main function:%lu:%lx\n", (unsigned long)reloc_main, (unsigned long)reloc_main);
	 
	 //Call the relocated main function

          asm volatile("mov %0, %%rax;"
		       "xor %%rdi, %%rdi;"
                       "callq *%%rax;"
		       :
		       : "m" (reloc_main)
		       :"rdi", "rax", "memory"
		      ); 
	  exit(0);		  
   }	   
   

   /*We check the RIP location after code relocation and call a couple of ``useless'' functions*/

   unsigned long now_c;
   unsigned folded;
   asm volatile("lea (%%rip), %%rax;"
		"mov %%rax, %0;"
	        :"=m" (now_c)
	        :
	        :"rax", "memory"
	        );	
   printf("Now RIP:%lu:%lx\n", now_c, now_c);
   folded = call_f1(now_c);
   printf("Folded number: %x\n", folded);
   return 0;  
}
