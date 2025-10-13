#include<hacks.h>
#include<sigexit.h>
#include<entry.h>
#include<lib.h>
#include<context.h>
#include<memory.h>

struct hack_config{
	             long cur_hack_config;
		     u64 usr_handler_addr;
};

//Used to store the current hack configuration
static struct hack_config hconfig = {-1, -1};


//system call handler to configure the hack sematics
//user space connection is already created, You need to
//store the hack semantics in the 'hconfig' structure (after validation)
//which will be used when division-by-zero occurs

long sys_config_hs(struct exec_context *ctx, long hack_mode, void *uhaddr)
{
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
		hconfig.cur_hack_config=-1129;
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
		if(regs->rcx==0){//this is not needed
			regs->rax=0;
			regs->rcx=1;
		}	
	}	
	else if(hconfig.cur_hack_config==DIV_ZERO_SKIP){
		regs->entry_rip+=3;
//		printk("\nregs ka do_div_by_zero mai memory yeh hai: %x\n",regs);
	}
	else if(hconfig.cur_hack_config==DIV_ZERO_USH_EXIT){
		regs->rdi=regs->entry_rip;
		regs->entry_rip=hconfig.usr_handler_addr;
	}	
	else if(hconfig.cur_hack_config==DIV_ZERO_SKIP_FUNC){
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
		do_exit(0);
	}
	
    return 0;   	
} 
