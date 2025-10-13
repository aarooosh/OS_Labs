#include<hacks.h>
#include<sigexit.h>
#include<entry.h>
#include<lib.h>
#include<context.h>
#include<memory.h>

struct hack_config{
			//struct to store the hack configuration and user handler function's address
	        long cur_hack_config;
		    u64 usr_handler_addr;
};

//Used to store the current hack configuration which is defaulted to -1,-1
static struct hack_config hconfig = {-1, -1};


//system call handler to configure the hack sematics
//user space connection is already created, You need to
//store the hack semantics in the 'hconfig' structure (after validation)
//which will be used when division-by-zero occurs

long sys_config_hs(struct exec_context *ctx, long hack_mode, void *uhaddr)
{
	//this fucntion configures the value in the struct hconfig after validating
	//a lot of the ifs are superflous , we could've just stored it after an OR
	if(hack_mode==DIV_ZERO_OPER_CHANGE){
		hconfig.cur_hack_config=hack_mode;	
	}
	else if(hack_mode==DIV_ZERO_SKIP){
		hconfig.cur_hack_config=hack_mode;	
//		printk("\nos_rsp ka memory yeh hai: %x\n", ctx->os_rsp);
//		printk("\nregs ka memory yeh hai: %x\n",&(ctx->regs));
	}
	else if(hack_mode==DIV_ZERO_SKIP_FUNC){
		hconfig.cur_hack_config=hack_mode;	
	}
	else if(hack_mode==DIV_ZERO_MAX){
		hconfig.cur_hack_config=hack_mode;	
	}
	else if(hack_mode==DIV_ZERO_USH_EXIT){
		hconfig.cur_hack_config=hack_mode;	
		if(ctx->mms[0].start<=(u64)uhaddr&&ctx->mms[0].end>(u64)uhaddr)
		{
			hconfig.usr_handler_addr=(u64)uhaddr;
		}
		else{
			hconfig.cur_hack_config=-1129;
			return -EINVAL;
		}

		//if(check valid code address)
	}
	else{
		hconfig.cur_hack_config=-1129; //this is set to -1129 a randomly chosen invalid config number
		return -EINVAL;
	}
	
    return 0; 
}


/*This is the handler for division by zero
 * 'regs' is a structure defined in include/context.h which
 * is already filled with the user execution state (by the asm handler)
 * and will be restored back when the function returns 
 *
 */
int do_div_by_zero(struct user_regs *regs)
{	
	if(hconfig.cur_hack_config==DIV_ZERO_OPER_CHANGE){
		if(regs->rcx==0){
			//the if is actually unnecessary since we assume correctness of the hardware error handler
			//i.e. rcx had to have been zero for this to have been called in the first place
			//Note : the assembly instruction does rax = rax/rcx (we got this from running the objdump)
			//since the q says OPERCHANGE , we change the value of the registers so that when the hardware executes (remember !
			//it's going to come back to the same div instruction since we've not changed the register of Instruction Pointer!)
			//when it executes that again , it's a valid (although tampered) division
			regs->rax=0;
			regs->rcx=1;
		}	
	}	
	else if(hconfig.cur_hack_config==DIV_ZERO_SKIP){
		regs->entry_rip+=3;
		//this is because the instruction consumed 3bytes (i guess this x86 64 has variable length instructions)
		//note : the skipped instruction is immediate div (idiv) of rax/rcx but the values of rax and rcx have been
		//populated in previous instructions ! so those don't change ! Be vigilant !
//		printk("\nregs ka do_div_by_zero mai memory yeh hai: %x\n",regs);
	}
	else if(hconfig.cur_hack_config==DIV_ZERO_USH_EXIT){
		//note : when we call a function , it looks for the passed arguments at standard locations (since they are being passed
		//from external to the function right ?) which is rdi,rci,r8,r9 etc (i forget the exact ones)
		//we're assured that the user handler will call exit itself so we don't have to worry about exiting the process
		//but if we didn't have this assurance , i'm not too sure how we would've done it?
		//because if it's a dummy handler which does nothing , it's possible to get stuck in an infinite loop
		//perhaps we could have a check of if the same process at the same instruction is causing this error and kill that
		//process from the OS handler?
		regs->rdi=regs->entry_rip;
		regs->entry_rip=hconfig.usr_handler_addr;
	}	
	else if(hconfig.cur_hack_config==DIV_ZERO_SKIP_FUNC){
		//some of this poointer gymnastics is superflous
		/**
|			  |
| 			  |
|------------ | <----- rsp
| locals 	  |
| for func2   |
| ... 		  |
--------------- <------rbp
| saved rbp   |
---------------
|RetAddr_func1|
---------------
|			  |
| Stack Frame |
| for func1   |
| ...         |

now the new base pointer value is set to the saved rbp which can be found by dereferencing the current rbp value
the instruction pointer now points to RetAddr_func1 so that the rest of the function is skipped
entry_rsp i.e. stack pointer at time of entry back into the current context should be set to rbp + 2 spaces , or rbp+16
since it's x86 64 (8 bytes)
it's also expected that the skipped function returns 1 and again , since it's returning to a space outside it's 
original domain , it writes the value to a standard register , in this case it's rax
hence we manually set rax=1

			*/
		u64 a=regs->rbp;// meditate on it
	    //regs->rbp=*(u64*)regs->rbp;
		u64* p=(u64*)regs->rbp;
		regs->rbp=*(u64*)p;
		regs->entry_rip=*(p+1);
		//regs->rbp=*(u64*)a;
		//regs->entry_rip=*(u64*)a+1;
		regs->entry_rsp=a+16;
		regs->rax = 1;	
		//regs->entry_rsp=a+2;	
	}
	else{
		printk("Error...exiting\n");
		//question specifies this !!! pay attention
		do_exit(0);
	}	
    return 0;   	
} 
