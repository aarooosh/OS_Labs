#include<context.h>
#include<page.h>
#include<memory.h>
#include<lib.h>


//////////////////////  Q1: RR Scheduling   ///////////////////////////////////////
//args:
//      ctx: new exec_context to be added in the linked list
//		the time delta check to call this scheduler process is apparently already dealt with
//		as the goal of this question is just to manipulate the linked list of processes that 
//		are ready/running
//
//		also remember that \n after prints is always a good idea as it forces flushing of the print buffer

void rr_add_context(struct exec_context *ctx)
{
      /*TODO*/
	
	struct exec_context *curr=rr_list_head;
	if(curr==NULL){
		//i.e. no process is ready/running , perhaps when OS boots up ?
		//ctx is the incoming process that has been scheduled , so if NULL directly point to 
		//ctx. We also force ctx.next = NULL because we're the ones dealing with the linked list logic
		//and as long as we account for this in later steps , we should be good
		//i.e. this is not 'losing' information as such
		rr_list_head=ctx;
		ctx->next=NULL;
		//returning as the shceduler functionality is done
		return;
	}
	//if not NULL loop till the end , find the last process and then add this new one to the tail
	while(curr->next!=NULL){
		curr=curr->next;
	}
	ctx->next=NULL;
	curr->next=ctx;
	//this concludes all cases for add schedule
	return;
}

//args:
//      ctx: exec_context to be removed from the linked list
void rr_remove_context(struct exec_context *ctx)
{
      /*TODO*/
	//note : don't set next to NULL while processing here , as pick next is called 
	//		 implicitly after this (already implemented) and hence if set to NULL 
	//		 it will fail to pick the next process.

	//idea is to just skip the node in the linked list
	//caveats : moving the rr_list_head pointer when needed
	//			when the only process in the linked list is removed , set head to NULL
	//			normal case skipping , but checking by pid to be safe
	
	struct exec_context *curr=rr_list_head;
	
	//ideally we can directly check pointer equality as i don't think multiple copies of PCB will ever be stored
	//but perhaps a safer way to go about it is using pid checking
	
	//to clarify : the below if is for the case where the process to be removed is the one pointed to by rr_list_head
	//			   as we'd have to change the rr_list_head pointer as well.
	if(rr_list_head->pid==ctx->pid){
		rr_list_head=rr_list_head->next;
		return;
	}

	//i feel this bit of code is redundant (not sure why we did it in the lab)
	//this only enters the if block if rr_list_head is the only process left 
	//in the linked list , and the fact that remove is called implies that it is
	//the one being removed. But the above if block already catches this , so i'm
	//not sure what new is being caught by this block

	if(rr_list_head->next==NULL){	  
		rr_list_head=NULL;
		return;
	}

	//normal skipping stuff , looking one node ahead (->next) because we want both the previous and next node
	//to implement the skipping logic
	
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
    /*TODO
	1. if empty schedule swapper
	2. if end of list pick the head
	3. normal case just choose the next process
	*/
	
	if(rr_list_head==NULL){
	// head being NULL implies no process and pick next process should pick the swapper/idle/pid(0) process
	//below print statement helped us figure out qns.txt
	//	printk("Swapper Process Got Scheduled\n");
		return get_ctx_by_pid(0);	
	}
	struct exec_context *coming=ctx->next;
	if(coming==NULL){
		//if next process is NULL pick the head process
		//this is safe because if only one process exists in the LL
		//rr_list_head still points to this , so it picks the same
		//process safely
		// remember ctx (and this whole function is called by) the outgoing process
		return rr_list_head;
	}
	else{
		//normal case just return next
		return coming;
	}

	//sabse next dekho, null hai ya nahi
	//then rr_list head dekho, if same then only one, reschedule

	//swapper mai if pid=0, pick the head, if the head is empty return ccurrent
	//i think the below return is also redundant
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

