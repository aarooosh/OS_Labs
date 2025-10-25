#include<context.h>
#include<page.h>
#include<memory.h>
#include<lib.h>


//////////////////////  Q1: RR Scheduling   ///////////////////////////////////////
//args:
//      ctx: new exec_context to be added in the linked list
void rr_add_context(struct exec_context *ctx)
{
      /*TODO*/
	//add context if its null
	struct exec_context *curr=rr_list_head;
	if(curr==NULL){
		rr_list_head=ctx;//confusion, would curr=ctx work? or *'s both side 
			   //safest is rr_list_head=ctx
		ctx->next=NULL;
		return;
	}
	while(curr->next!=NULL){
		curr=curr->next;
	}
	ctx->next=NULL;
	curr->next=ctx;
//not circling back to head, keep this in mind
	return;
}

//args:
//      ctx: exec_context to be removed from the linked list
void rr_remove_context(struct exec_context *ctx)
{
      /*TODO*/
	struct exec_context *curr=rr_list_head;
	//direct equate kar sakte hai, ya should we check by  pid, this is safer hence doing this
	//very smar tfigure out by lord aarush: we also have to update rr_list_head if wohi remove kar rahe hai 
	if(rr_list_head->pid==ctx->pid){
		rr_list_head=rr_list_head->next;
		return;
	}
	if(rr_list_head->next==NULL){//considering valid calls only
				  
		rr_list_head=NULL;
		return;
	}
	while((curr->next)->pid!=ctx->pid){
		curr=curr->next;
	}
	curr->next=(curr->next)->next;

      return;
}

//args:
//      ctx: exec_context corresponding the currently running process
struct exec_context *rr_pick_next_context(struct exec_context *ctx)
{
    /*TODO*/

	if(rr_list_head==NULL){
	//if empty, if head = null, and swapper hi wapas dedo, hence below function is usefull
	//	printk("Swapper Process Got Scheduled\n");
		return get_ctx_by_pid(0);	
	}
	struct exec_context *coming=ctx->next;
	if(coming==NULL){
		//what does start looking for process from the head
		return rr_list_head;
	}
	else{
		return coming;
	}

	//sabse next dekho, null hai ya nahi
	//then rr_list head dekho, if same then only one, reschedule

	//swapper mai if pid=0, pick the head, if the head is empty return ccurrent
     return get_ctx_by_pid(0);
}

//////////////////////  Q2: Get the PAGE TABLE details for given address   ///////////////////////////////////////


//args:
//      ctx: exec_context corresponding the currently running process
//      addr: address for which the PAGE TABLE details are to be printed

int do_walk_pt(struct exec_context *ctx, unsigned long addr)
{
    u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
    /*TODO*/
    u64 valid=1;
   // valid=(((u64)vaddr_base)&1); cheating a little assuming CR3 to valid hi hoga
    u64 L1_entry=0, L2_entry=0, L3_entry=0, L4_entry=0;
    if(valid){
    	u64 L1_pfn=(u64)vaddr_base;//for now assuming this is page aligned, thisis cr3 and assuming that ismai se direct virtual addr milega, and * of this gives the entry itself
	//so this is directly the address to the page table
	u64 L1_offset=((addr & PGD_MASK) >> PGD_SHIFT);
	//yeh to offset number hai, we need to multiply by size
	
	//aligning L1 pfn
	//L1_pfn=(L1_pfn>>9)<<9;

	u64* L1_final_addr=(u64*)(L1_pfn+L1_offset*8);
	//abb ismai kya hai woh dekhna hai na

	L1_entry=*L1_final_addr;
	//not too sure about L1-entry addr
        if(L1_entry&1){vaddr_base = (u64 *)osmap(L1_entry>>12);//assuming this won't give us a faul
	printk("L1-entry addr: %x, L1-entry contents: %x, PFN: %x, Flags: %x\n", L1_final_addr, L1_entry, (L1_entry>>12),(L1_entry&0xFFF));}
	else{
    	printk("No L1 entry\n");
	valid = 0;
    }
    }
    else{
    	printk("No L1 entry\n");
    }

    //LEVEL 2
    /*TODO*/
    valid=L1_entry&1&valid;
  	//We need to also semantically add & invalid here for the previous entry, but since all these variables are set to 0 unless valid, we don't need to worry about it
    if(valid){
	    u64 L2_pfn=(u64)vaddr_base;
	    u64 L2_offset=((addr & PUD_MASK) >> PUD_SHIFT);
	
	    u64* L2_final_addr=(u64*)(L2_pfn+L2_offset*8);
	    //u64* L2_pte=(u64*)osmap(L2_final_addr); pls  take care comment better

	    L2_entry=*L2_final_addr;
        if(L2_entry&1){vaddr_base = (u64 *)osmap(L2_entry>>12);//assuming this won't give us a fault
		printk("L2-entry addr: %x, L2-entry contents: %x, PFN: %x, Flags: %x\n", L2_final_addr, L2_entry, (L2_entry>>12),(L2_entry&0xFFF));}
	else{
		valid = 0;
    	printk("No L2 entry\n");
    }
    }
    else{
    	printk("No L2 entry\n");
    }
    //LEVEL 3
    valid=L2_entry&1&valid;
    if(valid){
	    u64 L3_pfn=(u64)vaddr_base;
	    u64 L3_offset=((addr & PMD_MASK) >> PMD_SHIFT);
	
	    u64* L3_final_addr=(u64*)(L3_pfn+L3_offset*8);

	    L3_entry=*L3_final_addr;
        if(L3_entry&1){vaddr_base = (u64 *)osmap(L3_entry>>12);//assuming this won't give us a fault
		printk("L3-entry addr: %x, L3-entry contents: %x, PFN: %x, Flags: %x\n", L3_final_addr, L3_entry, (L3_entry>>12),(L3_entry&0xFFF));}
	else{
		valid = 0;
    	printk("No L3 entry\n");
    }
    }
    else{
    	printk("No L3 entry\n");
    }
    //LEVEL 4
    valid=L3_entry&1;
    if(valid){
	    u64 L4_pfn=(u64)vaddr_base;
	    u64 L4_offset=((addr & PTE_MASK) >> PTE_SHIFT);
	
	    u64* L4_final_addr=(u64*)(L4_pfn+L4_offset*8);

	    L4_entry=*L4_final_addr;
        if(L4_entry&1){vaddr_base = (u64 *)osmap(L4_entry>>12);//assuming this won't give us a fault
		printk("L4-entry addr: %x, L4-entry contents: %x, PFN: %x, Flags: %x\n", L4_final_addr, L4_entry, (L4_entry>>12),(L4_entry&0xFFF));}
    	else{
		valid = 0;
		printk("No L4 entry\n");
	}
    }
    else{
    	printk("No L4 entry\n");
    }

    return 0;
}

