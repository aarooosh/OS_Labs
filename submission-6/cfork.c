#include <fork.h>
#include <page.h>
#include <mmap.h>
#include <apic.h>
#define L1_MASK 0xFF8000000000
#define L2_MASK 0x007FC0000000
#define L3_MASK 0x00003FE00000
#define L4_MASK 0x0000001FF000
#define L1_SHIFT 39
#define L2_SHIFT 30
#define L3_SHIFT 21
#define L4_SHIFT 12

//The masks are to select 9 bits at a time for the page table walk
//L1 selects the first 9 bits of PGD offset from the address , L2 the next so on...
//Last 12 bits of address are used for addressing specific bytes inside the Page PFN
//The shifts are set accordingly so that the bits are moved to appropriate locations




/* #################################################*/

static inline void invlpg(unsigned long addr) {
	//This function invalidates the TLB (Translation Lookaside Buffer) entry associated with addr
	//A small point to note , here , gemOS TLBs are NOT ASID compliant !!
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
/**
 * cfork system call implemenations
 */


//The Idea we had was to traverse the parent and the child one level at a time SIMULTANEOUSLY
//Sir's idea was rather different , as he noted that the main two functionalities we'd be needing are a get_pte(ctx,addr)
//and a map_physical_page(ctx , addr , pfn[for copy purposes] , flags[for alloc])
//This is probably a way cleaner way to do things 

//In our code we sweep through each of the allocated areas by the parent , calling create_tree_entry on each of the addresses
//create_tree_entry walks through each step simultaneously for the parent and the child , stopping if the parent page does not
//exist , allocating a new child page if it doesn't exist where it should (i.e. parent page has been allocated but not child page)
//If the child page already exists , we just continue stepping through , and this process continues until we reach the final level
//where we point the child PTE to the parent PTE so that they point to the same physical page.
//We also change the write permissions to both of them to be zero (this will be relevant later , we had made a mistake here)
//so that we can generate a CoW fault anytime someone tries to write.

int create_tree_entry(struct exec_context* ctx, struct exec_context* new_ctx, u64 curr){
	
	u64 *parent_base = (u64 *)osmap(ctx->pgd);
	invlpg(curr);	
	u64 *child_base = (u64 *)osmap(new_ctx->pgd);
			
			//parent_base is the virtual address of CR3 ka address
			//i think the invlpg here is incorrect , but TLB flush will never really cause an error as such
			//we'd prefer to invalidate the TLB entry only when we're changing the PFN , maybe not from the get go
			//child_base is the virtual address of CR3 ka address

/* ################################################# DEBUG PRINTS #################################################*/
	//FIRST LEVEL
	//printk("CR3 Parent: %x, Child: %x\n", parent_base, child_base);
	//we need to increase by multiples of 8

/* ################################################# FATAL ERROR (1) #################################################*/
	
	//CORRECTED : 
		int level_1_offset=((curr&L1_MASK)>>L1_SHIFT);
		u64 parent_entry=*(parent_base+level_1_offset);
	//INCORRECT :
		//int level_1_offset=8*((curr&L1_MASK)>>L1_SHIFT);
		//u64 parent_entry=*(parent_base+level_1_offset);
	
	//This is to calculate the level 1 offset value and the entry
	//we DO NOT need to shift by multiples of 8 , as when we cast it , it is automatically treated
	//as a u64 pointer !!
	//the dereference gives us the value stored at the L1 location , of the form L2_PFN,FLAGS
	
	
	//parent page itself didn't exist, woh lazy allocation bs, so aage badhne ki hi zarurat nahi
/* ################################################# FATAL ERROR (2) #################################################*/

	//CORRECTED:
		if((parent_entry%2==0)){
			return 0;
		}
		if(*(child_base+level_1_offset)%2==0){
			*(child_base+level_1_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xF9F&parent_entry);
			bzero(*(child_base+level_1_offset));
		}

		//if the parent entry does not exist , return 0;
		//otherwise , if the child doesn't exist , allocate page using parent flags
		//The mask here is F9F (1111-1001-1111) so as to not copy Dirty Bit and Accessed Bits 
	
	//INCORRECT:
		// if(*(child_base+level_1_offset)%2==0&&(parent_entry%2!=0)){//check this
		// 				      //this signifies if the page exists or not
		// 	*(child_base+level_1_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xFFF&parent_entry);
		// 	//assuming this will deal with the present bit and all
		// 	//and increase ref_count
		// }
		// else{
		// 	return 0;
		// }

	//That was incorrect because we made a mistake in the condition checking
	//if the child page existed , we were enetering into the else part , and instead of continuing forward
	//the else part would just kill the function , leading to all sorts of skulduggery

	parent_base=(u64 *)osmap((parent_entry>>12));
	u64 child_entry=*(child_base+level_1_offset);
	child_base=(u64 *)osmap((child_entry>>12));

	//These are the traverse step
	//For the next level , we're setting the base to the virtual address corresponding to the 
	//entry in the L1 level

/* ################################################# DEBUG PRINTS #################################################*/
		//	parent_base = (u64 *)osmap(*((parent_base>>12)<<12+level_1_offset));
		//	child_base = (u64 *)osmap(*((child_base>>12)<<12+level_1_offset));
		//	printk("PGD_T Parent: %x, Child: %x\n", parent_base, child_base);
	
	//SECOND LEVEL
	int level_2_offset=((curr&L2_MASK)>>L2_SHIFT); 
	parent_entry=*(parent_base+level_2_offset);

	if((parent_entry%2==0)){
			return 0;
		}
		if(*(child_base+level_2_offset)%2==0){
			*(child_base+level_2_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xF9F&parent_entry);
			bzero(*(child_base+level_2_offset));
		}
	
	parent_base=(u64 *)osmap((parent_entry>>12));
	child_entry=*(child_base+level_2_offset);
	child_base=(u64 *)osmap((child_entry>>12));
	
	//THIRD LEVEL
	int level_3_offset=((curr&L3_MASK)>>L3_SHIFT);
	parent_entry=*(parent_base+level_3_offset);

	if((parent_entry%2==0)){
			return 0;
		}
		if(*(child_base+level_3_offset)%2==0){
			*(child_base+level_3_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xF9F&parent_entry);
			bzero(*(child_base+level_3_offset));
		}
	
	parent_base=(u64 *)osmap((parent_entry>>12));
	child_entry=*(child_base+level_3_offset);
	child_base=(u64 *)osmap((child_entry>>12));
	

	//FOURTH LEVEL
	//don't check for the last level, direct pte same and modify
	int level_4_offset=((curr&L4_MASK)>>L4_SHIFT);
	
/* ################################################# DEBUG PRINTS #################################################*/
	//	parent_entry=*(parent_base+level_4_offset);
	//	parent_base=(u64 *)osmap((parent_entry>>12)<<12);
	//	child_entry=*(child_base+level_3_offset);
	//	child_base=(u64 *)osmap((child_entry>>12)<<12);

	u64* parent_entry_pointer =parent_base+level_4_offset;
	u64* child_entry_pointer =child_base+level_4_offset;
	//These are already virtual addresses !

	if(*parent_entry_pointer%2!=0){
/* ################################################# FATAL ERROR (3) #################################################*/
		//CORRECTED:
			*parent_entry_pointer=*parent_entry_pointer&(0xFFFFFFFFFFFFFFF7);
		//INCORRECT:
			//*parent_entry_pointer=*parent_entry_pointer&(0xFFFFFFFFFFFFFFFB);//xor it and turn the first bit 0
			//this was the wrong mask you doofus !!!!

		//pointing the child entry to the parent entry (physical)
		*(child_entry_pointer)=*parent_entry_pointer;
		//making sure to update refcount
		get_pfn(((*parent_entry_pointer)>>12));
	}
	else{
	//	printk("las page not present\n");
		return 0;
	}
	return 0;
}

long do_cfork(){
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
     /* Do not modify above lines
     * 
     * */   
     /*--------------------- Your code [start]---------------*/
     
    u32 new_pid=new_ctx->pid;
	*new_ctx=*ctx;
	new_ctx->ppid=ctx->pid;
	new_ctx->pid=new_pid;
	new_ctx->pgd=os_pfn_alloc(OS_PT_REG);
	pid = new_pid;
	new_ctx->regs.rax = 0;
	ctx->regs.rax = pid;

	//finished copying all basic fields from parent to child contexts
	//now creating a new tree
	//for code, rodata, data segs
	u64 start,end;
/* ################################################# FATAL ERROR (4) #################################################*/
	//CORRECTED:
		for(int i=0;i<MAX_MM_SEGS-1;i++){
				start=ctx->mms[i].start;
				if(i == MM_SEG_STACK){
					end = ctx->mms[i].end;
				}
				else{
					end=ctx->mms[i].next_free;
				}
				while(start<end){
					int if_okay=create_tree_entry(ctx, new_ctx, start);
					start+=4096;
				}
			}

	//INCORRECT: we had bungled up the stack start and end stuff
		// for(int i=0;i<MAX_MM_SEGS-1;i++){
		// 	start=ctx->mms[i].start;
		// 	end=ctx->mms[i].next_free;
		// 	while(start<end){
		// 		int if_okay=create_tree_entry(ctx, new_ctx, start);
		// 		start+=4096;
		// 	}
		// }
		// start=ctx->mms[MAX_MM_SEGS-1].start;
		// end=ctx->mms[MAX_MM_SEGS-1].end;
		// if(start>end){
		// 		while(start>end){
		// 		int if_okay=create_tree_entry(ctx, new_ctx, start);
		// 		//	printk("mms start:%x, end: %x create_tree succeeded ? : %x\n", start, end,if_okay);
		// 		start-=4096;
		// 	}
		// }
	
		// else{
		// 	while(start<end){
		// 		int if_okay=create_tree_entry(ctx, new_ctx, start);
		// 		//	printk("mms start:%x, end: %x create_tree succeeded ? : %x\n", start, end,if_okay);
		// 		start+=4096;
		// 	}
		// }


//VMAREADEEPCOPYNAHIKIYA!..
	
	struct vm_area* head=ctx->vm_area;
	
/* ################################################# FATAL ERROR (5) #################################################*/
	//CORRECTED: add this if statement 
		if(head == NULL){
			new_ctx->vm_area = NULL;
			goto skip;
		}
	//we'd forgot to check if head = NULL 

	//traverse and create tree entries in the child
	struct vm_area* new_head=os_alloc(sizeof(struct vm_area));
	struct vm_area* yummy = new_head;
	//new_head will be the traversing pointer , yummy will stay at the head point
	new_ctx->vm_area = yummy;
	while(head!=NULL){
		start=head->vm_start;
		end=head->vm_end;
		while(start<end){
			int if_okay=create_tree_entry(ctx,new_ctx,start);
			start+=4096;//move page by page ! crucial catch by Shrey
	//		printk("tehelka omlette mms start:%x, end: %x create tree succeeded ? %x\n", start, end,if_okay);
		}
		head=head->vm_next;
	}

	//initialise the child VMAs to our newly traversed/created tree
	head = ctx->vm_area;
	while(head!=NULL){
		//struct vm_area *vm = os_alloc(sizeof(struct vm_area));
		new_head->vm_start=head->vm_start;
		new_head->vm_end=head->vm_end;
		new_head->access_flags=head->access_flags;
		head = head->vm_next;
		if(head==NULL){
			new_head->vm_next=NULL;
		}
		else{
			new_head->vm_next = os_alloc(sizeof(struct vm_area));
			new_head=new_head->vm_next;
		}
	}
//	printk("deep copy of VMA succeeded\n");


skip:
     /*--------------------- Your code [end] ----------------*/
    
     /*
     * The remaining part must not be changed
     */
    //Check when free
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    //What exactly is stored in the OS Page Tables? Yaha pe bhi CoW kyu nahi?
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
    reset_timer();//?

  //  printk("returning clone %d\n", new_ctx->regs.rax);
   // printk("returning clone parent %d\n", ctx->regs.rax);
    return pid;
}


/* Cow fault handling, for the entire user address space
 * For address belonging to memory segments (i.e., stack, data) 
 * it is called when there is a CoW violation in these areas. 
 */

long handle_cow_fault(struct exec_context *current, u64 vaddr, int access_flags)
{
	//cow fault izzat se call hua hai
	long retval = -1;
	u64 *parent_base = (u64 *)osmap(current->pgd);
	u64 curr=vaddr;
	//FIRST LEVEL
	int level_1_offset=((curr&L1_MASK)>>L1_SHIFT);
	u64 parent_entry=*(parent_base+level_1_offset);
	parent_base = (u64 *)osmap((parent_entry>>12));
	//SECOND LEVEL
	int level_2_offset=((curr&L2_MASK)>>L2_SHIFT);
	parent_entry=*(parent_base+level_2_offset);
	parent_base = (u64 *)osmap((parent_entry>>12));
	//THIRD LEVEL
	int level_3_offset=((curr&L3_MASK)>>L3_SHIFT);
	parent_entry=*(parent_base+level_3_offset);
	parent_base = (u64 *)osmap((parent_entry>>12));
	//FOURTH LEVEL
	int level_4_offset=((curr&L4_MASK)>>L4_SHIFT);
	u64* parent_entry_pointer=parent_base+level_4_offset;

//	parent_base = (u64 *)osmap((parent_entry>>12)<<12);

	//bzero

//	printk("level 4 \n");
//	*parent_base = *(parent_base+level_4_offset);
	if(access_flags & PROT_READ){
	//probably better permission checking can be done
	//assuming read to hoga hi, we just check W in xWr
	//right now, agar fault to naya page directly, not freeing, and don;t care about wastage	
/* ################################################# FATAL ERROR (6) #################################################*/
	//CORRECTED:
		s8 refc = get_pfn((old_entry>>12));
		if(refc == 2){
			//last referrer , don't dealloc and don't make a new page
			//have to go through get_pfn gymnastics as put_pfn throws an error if 0 refcount
			pass;
		}
		else{
			put_pfn((old_entry>>12));
			*(parent_entry_pointer)=(os_pfn_alloc(USER_REG)<<12)+(old_entry&0xFFF);
			bzero(*(parent_entry_pointer)); //good practise to fill the area with zeroes
			*parent_entry_pointer=(*(parent_entry_pointer)|8);
			parent_entry=*parent_entry_pointer;
			u64* old_base = (u64 *)osmap((old_entry>>12));
			u64* new_base = (u64 *)osmap((parent_entry>>12));
			memcpy(new_base,old_base,4096);
			invlpg(old_entry>>12);
		}
		put_pfn((old_entry>>12));
		
	//INCORRECT:
		// u64 old_entry=*(parent_entry_pointer);
		// put_pfn((old_entry>>12));
		// *(parent_entry_pointer)=(os_pfn_alloc(USER_REG)<<12)+(old_entry&0xFFF);
		// *parent_entry_pointer=(*(parent_entry_pointer)|8);
		// parent_entry=*parent_entry_pointer;
		// u64* old_base = (u64 *)osmap((old_entry>>12));
		// u64* new_base = (u64 *)osmap((parent_entry>>12));
		// memcpy(new_base,old_base,4096);
//		printk("oldbase %x newbase %x parent entry %x old entrry %x \n",old_base,new_base,parent_entry,old_entry);
		return 1;
	}

	else{
		return -1;
	}
}
