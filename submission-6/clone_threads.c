#include<clone_threads.h>
#include<entry.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<mmap.h>
#include<fork.h>
#include<page.h>

extern int destroy_user_mappings(struct exec_context *ctx);

static void set_kstack_of_thread(struct exec_context *ctx)
{
   ctx->os_stack_pfn = os_pfn_alloc(OS_PT_REG);
   //Treat this as a page table reg for OS, we used USER_REG
   ctx->os_rsp = (((u64) ctx->os_stack_pfn) << PAGE_SHIFT) + PAGE_SIZE;
   //Aarush Godman: We're getting a pfn, address to nahi, hence we shift, now stack niche to start hota hai (it's not upar), we add PAGE_SIZE
   stats->num_processes++;
   //global var, serch mor
   ctx->type = EXEC_CTX_USER_TH;	
}

//XXX Do not modify anything above this line

/*
  system call handler for clone, create thread like execution contexts
*/
long do_clone(void *th_func, void *user_stack, void *user_arg) 
{
  int ctr;
  struct exec_context *new_ctx = get_new_ctx();  //This is to be used for the newly created thread
  struct exec_context *ctx = get_current_ctx();
  u32 pid = new_ctx->pid;
  struct thread *n_thread;

  if(!ctx->ctx_threads){  // This is the first thread
          ctx->ctx_threads = os_alloc(sizeof(struct ctx_thread_info));
          bzero((char *)ctx->ctx_threads, sizeof(struct ctx_thread_info));
          ctx->ctx_threads->pid = ctx->pid;
  }

 /* XXX Do not modify anything above. Your implementation goes here */

 // TODO your code goes here
	//THREAD_INFO
	
	new_ctx->ctx_threads=NULL;
//	ctx->ctx_threads->threads.pid=new_ctx->pid;
	n_thread=find_unused_thread(ctx);//not newctx
	n_thread->status=TH_USED;
	n_thread->pid=new_ctx->pid;
	n_thread->parent_ctx=ctx;	

	new_ctx->ppid=ctx->pid;
//alr done
//new_ctx->type=EXEC_CTX_USER_TH;
	new_ctx->state=READY;//NEW is irrelevant, used for some other function, once this thread is created it is ready to be used hence..
	new_ctx->used_mem=ctx->used_mem;	
	//check what this is	
	new_ctx->pgd=ctx->pgd;
	for(int i=0;i<MAX_MM_SEGS;i++){
		new_ctx->mms[i]=ctx->mms[i];//check what this is
	}
// BLUNDAHnew_ctx->mms = ctx->mms;
	new_ctx->vm_area = ctx->vm_area;
	for(int i=0;i<CNAME_MAX;i++){
		new_ctx->name[i]=ctx->name[i];//check what this is
	}
	for(int i=0;i<MAX_SIGNALS;i++){
		new_ctx->sighandlers[i]=ctx->sighandlers[i];//check what this is
	}
new_ctx->pending_signal_bitmap=ctx->pending_signal_bitmap;
	// BLUNDAH new_ctx->sighandlers=ctx->sighandlers;
	new_ctx->ticks_to_sleep=ctx->ticks_to_sleep;
	new_ctx->alarm_config_time=ctx->alarm_config_time;
	new_ctx->ticks_to_alarm=ctx->ticks_to_alarm;
	//not sure, inspiration from fork and that we don't want conflicts while writing
	//naye ka files also allocated only
//	new_ctx->files=ctx->files;
	for(int i=0;i<MAX_OPEN_FILES;i++){
		new_ctx->files[i]=ctx->files[i];//does this work? or do we have to do it in detail
	  //BLUNDAH dkwhy 	new_ctx->files[i]->ref_count++;
	}
	new_ctx->lock=ctx->lock;
	//abhi do process hai and dono ka next same cheez hai
	//think about this, is the round robin wrt threads or processes?
	new_ctx->next=ctx->next;

	//USELESS because any memory change done by the thread should reflect in the parent thread
/*	for(int i=0;i<MAX_MM_SEGS;i++){
		new_ctx->mms[i]=ctx->mms[i];//does this work? or do we have to do it in detail
	}
	vm_area* head=ctx->vm_area;
	while(head!=NULL){	
		vm_area* new_head=os_alloc(sizeof(struct vm_area));
		new_head->vm_start=head->vm_start;
		new_head->vm_end=head->vm_end;
		new_head->access_flags=head->access_flags;
		new_head->vm_next=NULL;
	}
*/


	//REGS AND INFO part
	
	new_ctx->regs=ctx->regs;
	new_ctx->regs.entry_rip=th_func;//code seg addr
	new_ctx->regs.entry_rsp=user_stack;
	new_ctx->regs.rbp=user_stack;//check this, saved_rbp abhi kuchh nahi, the thing that it points to is NULL
	new_ctx->regs.rdi=user_arg;
	

 //End of your logic
  
 //XXX The following two lines should be there. 
  
  set_kstack_of_thread(new_ctx);  //Allocate kstack for the thread
  return pid;
}



//handler for exit()
//void do_exit(u8 normal)
//{
  // return;
//}

// XXX Reference implementation for a process exit
// You can refer this to implement your version of do_exit

void do_exit(u8 normal) 
{
  int ctr;
  struct exec_context *ctx = get_current_ctx();
  struct exec_context *new_ctx;

 
  if(ctx->type!=EXEC_CTX_USER_TH){
	  do_file_exit(ctx);   // Cleanup the files, refcount kam karega
			       // cleanup of this process
	  destroy_user_mappings(ctx); 
	  do_vma_exit(ctx);

	  if(!put_pfn(ctx->pgd)) 
	  {
		  os_pfn_free(OS_PT_REG, ctx->pgd); 
	  } 
	  if(!put_pfn(ctx->os_stack_pfn))
	  {
		  os_pfn_free(OS_PT_REG, ctx->os_stack_pfn);
	  }
	  cleanup_all_threads(ctx);
 //XXX Now its fine as it is a single core system 
  }
  else
  {
  	handle_thread_exit(ctx,normal);
  }

   release_context(ctx); 
  new_ctx = pick_next_context(ctx);
  dprintk("Scheduling %s:%d [ptr = %x]\n", new_ctx->name, new_ctx->pid, new_ctx); 
  schedule(new_ctx);  //Calling from exit
}




////////////////////////////////////////////////////////// Semaphore implementation ////////////////////////////////////////////////////
//
//


// A spin lock implementation using cmpxchg
// XXX you can use it for implementing the semaphore
// Do not modify this code

static void spin_init(struct spinlock *spinlock)
{
	spinlock->value = 0;
	//printk("spinlock initialised\n");
}

static void spin_lock(struct spinlock *spinlock)
{
	unsigned long *addr = &(spinlock->value);

	asm volatile(
		"mov $1,  %%rcx;"
		"mov %0,  %%rdi;"
		"try: xor %%rax, %%rax;"
		"lock cmpxchg %%rcx, (%%rdi);"
		"jnz try;"
		:
		: "r"(addr)
		: "rcx", "rdi", "rax", "memory"
	);
}

static void spin_unlock(struct spinlock *spinlock)
{
	spinlock->value = 0;
}

static int init_sem_metadata_in_context(struct exec_context *ctx)
{
   if(ctx->lock){
	   printk("Already initialized MD. Call only for the first time\n");
	   return -1;
   }
   ctx->lock = (struct lock*) os_alloc(sizeof(struct lock) * MAX_LOCKS);
   if(ctx->lock == NULL){
			printk("[pid: %d]BUG: Out of memory!\n", ctx->pid);
                        return -1;
   }
	
   for(int i=0; i<MAX_LOCKS; i++)
			ctx->lock[i].state = LOCK_UNUSED;
}

// XXX Do not modify anything above this line

/*
  system call handler for semaphore creation
*/
int do_sem_init(struct exec_context *current, sem_t *sem_id, int value)
{
	if(current->lock == NULL){
		init_sem_metadata_in_context(current);
		for(int i=0; i<MAX_LOCKS; i++)
		{spin_init(&(current->lock[i].sem.lock));}
	}
        // TODO Your implementation goes here
	
	for(int i=0;i<MAX_LOCKS;i++){
			
		if(current->lock[i].state==LOCK_UNUSED){
			//in god(hindi) we trust
			spin_lock(&(current->lock[i].sem.lock));//what if dono ne unused dekh ke same position mai aye, one acquires the lock

			//we check again
			if(current->lock[i].state==LOCK_UNUSED){
				
				current->lock[i].id=sem_id;//for not lets assume that the bhalue is given
							   //we want it to be unique so what we simply do is address pass kardo, waise bhi unique hoga
				current->lock[i].state=LOCK_USED;
				current->lock[i].sem.value=value;
				current->lock[i].sem.wait_queue=NULL;
				spin_unlock(&(current->lock[i].sem.lock));
				//when this lock is released you'll never have lock_unused again
				return 0;
			}
			//dono mai release karo
			else{
				//hence you come here, you unlock and move on to the next free
				spin_unlock(&(current->lock[i].sem.lock));
				continue;
			}
		}
	}	


	return -EAGAIN;
}

/*
  system call handler for semaphore acquire
*/

int do_sem_wait(struct exec_context *current, sem_t *sem_id)
{
	for(int i=0;i<MAX_LOCKS;i++){
			
		if(current->lock[i].id==sem_id){
			spin_lock(&(current->lock[i].sem.lock));

			if(current->lock[i].sem.value>0){
				current->lock[i].sem.value--;
				
				spin_unlock(&(current->lock[i].sem.lock));
				return 0;
			}		
			else{	
							
				struct exec_context* new_ctx = pick_next_context(current);
				current->state=WAITING;
				if(current->lock[i].sem.wait_queue==NULL)
				{
					current->lock[i].sem.wait_queue=current;
					current->next=NULL;
				}
				else{
					struct exec_context *head=current->lock[i].sem.wait_queue;
					while(head->next!=NULL){
						head=head->next;
					}
					head->next=current;
					current->next=NULL;
				}

				spin_unlock(&(current->lock[i].sem.lock));
					//poblem
				schedule(new_ctx); 
			}
						
			return 0;
		}
	}	


	return -EAGAIN;

}

/*
  system call handler for semaphore release
*/
int do_sem_post(struct exec_context *current, sem_t *sem_id)
{
	for(int i=0;i<MAX_LOCKS;i++){
			
		if(current->lock[i].id==sem_id){
			spin_lock(&(current->lock[i].sem.lock));
			struct exec_context* new_ctx = pick_next_context(current);
			if(current->lock[i].sem.wait_queue==NULL)
			{
				current->lock[i].sem.value++;
				spin_unlock(&(current->lock[i].sem.lock));
				return 0;
			}
			else{
				struct exec_context *head=current->lock[i].sem.wait_queue;
				current->lock[i].sem.wait_queue=head->next;
				head->state=READY;
				spin_unlock(&(current->lock[i].sem.lock));
				return 0;
			}
		}
	}	


	return -EAGAIN;
}
