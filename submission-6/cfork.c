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






/* #################################################*/

static inline void invlpg(unsigned long addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
/**
 * cfork system call implemenations
 */

int create_tree_entry(struct exec_context* ctx, struct exec_context* new_ctx, u64 curr){
	u64 *parent_base = (u64 *)osmap(ctx->pgd);
	invlpg(curr);								 //	u64 parent_pte=*parent_base;
	//shuddhata assumed
	u64 *child_base = (u64 *)osmap(new_ctx->pgd);


	//FIRST LEVEL
//printk("CR3 Parent: %x, Child: %x\n", parent_base, child_base);
	//we need to increase by multiples of 8
	int level_1_offset=8*((curr&L1_MASK)>>L1_SHIFT);

	u64 parent_entry=*(parent_base+level_1_offset);
	//SIRF PFN DAALNA HAI, THE G in Aarush G. is for Godman
	//parent page itself didn't exist, woh lazy allocation bs, so aage badhne ki hi zarurat nahi
	if(*(child_base+level_1_offset)%2==0&&(parent_entry%2!=0)){//check this
					      //this signifies if the page exists or not
		*(child_base+level_1_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xFFF&parent_entry);
		//assuming this will deal with the present bit and all
		//and increase ref_count
	}
	else{
		return 0;
	}

	parent_base=(u64 *)osmap((parent_entry>>12));
	u64 child_entry=*(child_base+level_1_offset);
	child_base=(u64 *)osmap((child_entry>>12));

//	parent_base = (u64 *)osmap(*((parent_base>>12)<<12+level_1_offset));
//	child_base = (u64 *)osmap(*((child_base>>12)<<12+level_1_offset));

//	printk("PGD_T Parent: %x, Child: %x\n", parent_base, child_base);
	//SECOND LEVEL
	int level_2_offset=8*((curr&L2_MASK)>>L2_SHIFT);
	parent_entry=*(parent_base+level_2_offset);

	if(*(child_base+level_2_offset)%2==0&&(parent_entry%2!=0)){
		*(child_base+level_2_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xFFF&parent_entry);
	}
	else{
		return 0;
	}

	parent_base=(u64 *)osmap((parent_entry>>12));
	child_entry=*(child_base+level_2_offset);
	child_base=(u64 *)osmap((child_entry>>12));


	
	//THIRD LEVEL
//	printk("PUD Parent: %x, Child: %x\n", parent_base, child_base);
	int level_3_offset=8*((curr&L3_MASK)>>L3_SHIFT);
	parent_entry=*(parent_base+level_3_offset);

	if(*(child_base+level_3_offset)%2==0&&(parent_entry%2!=0)){
		*(child_base+level_3_offset)=(os_pfn_alloc(OS_PT_REG)<<12)+(0xFFF&parent_entry);
	}
	else{
		return 0;
	}

	parent_base=(u64 *)osmap((parent_entry>>12));
	child_entry=*(child_base+level_3_offset);
	child_base=(u64 *)osmap((child_entry>>12));
	

//	printk("PMD Parent: %x, Child: %x\n", parent_base, child_base);
	//FOURTH LEVEL
	//don't check for the last level, direct pte same and modify
	int level_4_offset=8*((curr&L4_MASK)>>L4_SHIFT);

//	parent_entry=*(parent_base+level_4_offset);
//	parent_base=(u64 *)osmap((parent_entry>>12)<<12);

//	child_entry=*(child_base+level_3_offset);
//	child_base=(u64 *)osmap((child_entry>>12)<<12);


	
	u64* parent_entry_pointer =parent_base+level_4_offset;
	u64* child_entry_pointer =child_base+level_4_offset;

	if(*parent_entry_pointer%2!=0){
		*parent_entry_pointer=*parent_entry_pointer&(0xFFFFFFFFFFFFFFFB);//xor it and turn the first bit 0
		*(child_entry_pointer)=*parent_entry_pointer;
		get_pfn(((*parent_entry_pointer)>>12));
	//	printk("pareent entry : %x child entry = %x \n",*parent_entry_pointer,*child_entry_pointer);
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
//	printk("copied pids and PGD\n");
	pid = new_pid;
	new_ctx->regs.rax = 0;
	ctx->regs.rax = pid;
	//Creating a new tree
	//for code, rodata, data segs
	u64 start,end;
	for(int i=0;i<MAX_MM_SEGS-1;i++){
		start=ctx->mms[i].start;
		end=ctx->mms[i].next_free;
//		printk("mms start:%x, end: %x\n", start, end);
		while(start<end){
			int if_okay=create_tree_entry(ctx, new_ctx, start);
			start+=4096;
		}
	}
	start=ctx->mms[MAX_MM_SEGS-1].start;
	end=ctx->mms[MAX_MM_SEGS-1].end;
	if(start>end){
			while(start>end){
			int if_okay=create_tree_entry(ctx, new_ctx, start);
			//	printk("mms start:%x, end: %x create_tree succeeded ? : %x\n", start, end,if_okay);
			start-=4096;
		}
	}

	else{
		while(start<end){
			int if_okay=create_tree_entry(ctx, new_ctx, start);
			//	printk("mms start:%x, end: %x create_tree succeeded ? : %x\n", start, end,if_okay);
			start+=4096;
		}

	}
//VMAREADEEPCOPYNAHIKIYA!..
	
	struct vm_area* head=ctx->vm_area;
	struct vm_area* new_head=os_alloc(sizeof(struct vm_area));
	struct vm_area* yummy = new_head;
	new_ctx->vm_area = yummy;
	while(head!=NULL){
		start=head->vm_start;
		end=head->vm_end;
		while(start<end){
			int if_okay=create_tree_entry(ctx,new_ctx,start);
			start+=4096;
	//		printk("tehelka omlette mms start:%x, end: %x create tree succeeded ? %x\n", start, end,if_okay);
		}
		head=head->vm_next;
	}

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
	int level_1_offset=8*((curr&L1_MASK)>>L1_SHIFT);
	u64 parent_entry=*(parent_base+level_1_offset);

	parent_base = (u64 *)osmap((parent_entry>>12));
	//SECOND LEVEL
//	printk("level 1 succeeded\n");
	int level_2_offset=8*((curr&L2_MASK)>>L2_SHIFT);
	parent_entry=*(parent_base+level_2_offset);


	parent_base = (u64 *)osmap((parent_entry>>12));
//	printk("level 2 succeeded\n");
	//THIRD LEVEL
	int level_3_offset=8*((curr&L3_MASK)>>L3_SHIFT);
	parent_entry=*(parent_base+level_3_offset);

	parent_base = (u64 *)osmap((parent_entry>>12));

//	printk("level 3 succeeded\n");
	//FOURTH LEVEL
	int level_4_offset=8*((curr&L4_MASK)>>L4_SHIFT);
	u64* parent_entry_pointer=parent_base+level_4_offset;

//	parent_base = (u64 *)osmap((parent_entry>>12)<<12);


//	printk("level 4 \n");
//	*parent_base = *(parent_base+level_4_offset);
	if(access_flags&2>0){//assuming read to hoga hi, we just check W in xWr
	//right now, agar fault to naya page directly, not freeing, and don;t care about wastage	
		u64 old_entry=*(parent_entry_pointer);
		put_pfn((old_entry>>12));
		*(parent_entry_pointer)=(os_pfn_alloc(USER_REG)<<12)+(old_entry&0xFFF);
		*parent_entry_pointer=(*(parent_entry_pointer)|8);
		parent_entry=*parent_entry_pointer;
		
		u64* old_base = (u64 *)osmap((old_entry>>12));
		u64* new_base = (u64 *)osmap((parent_entry>>12));
		
		memcpy(new_base,old_base,4096);
//		printk("oldbase %x newbase %x parent entry %x old entrry %x \n",old_base,new_base,parent_entry,old_entry);
		return 1;
	}

	else{
		return -1;
	}
}
