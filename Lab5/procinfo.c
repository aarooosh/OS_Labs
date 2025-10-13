#include<procinfo.h>
#include<lib.h>
#include<file.h>
#include<context.h>

static char* segment_names[MAX_MM_SEGS+1] = {"code", "rodata", "data", "stack", "invalid"}; 
static char* file_types[MAX_FILE_TYPE+1] = {"stdin", "stdout", "stderr", "reg", "pipe", "invalid"}; 

long get_process_info(struct exec_context *ctx, long cmd, char *ubuf, long len)
{
    long retval = -EINVAL;	
    if(cmd==GET_PINFO_GEN){
		//get general info about the process

		//checks if user memory addr is valid and length is sufficient to accomodate the struct general_info
    	if(ubuf!=NULL && len >=sizeof(struct general_info)){
		//assuming we can cast ubuf directly because its already allocated and has enough space
		
		struct general_info casted_ubuf; 
		//struct general_info* casted_ubuf= (struct general_info*)ubuf; 
		//filling up the values
		casted_ubuf.pid=ctx->pid;
		casted_ubuf.ppid=ctx->ppid;
		casted_ubuf.pcb_addr=(unsigned long)ctx;
		strcpy(casted_ubuf.pname, ctx->name);
		memcpy(ubuf,&casted_ubuf,sizeof(struct general_info));
		//remember ! we have to make a deep copy because the pointers will go out of scope after this is executed !
		//casted_ubuf->pname=ctx->name;
		retval=1;
		//printk("%d\n",retval);
	}
    }
     else if(cmd==GET_PINFO_FILE){
    	if(ubuf!=NULL){
		//first find the open files' info that we have to store to check against length later
		//printk("%d\n",cmd == GET_PINFO_GEN);
		//assuming we can cast ubuf directly because its already allocated and has enough space
		struct file** filelist=ctx->files;
		struct file_info filearr[MAX_OPEN_FILES];
		int used=0;
		//loop over all possible fds and retrieve releveant info from the ones that are used
		for(int i=0;i<MAX_OPEN_FILES;i++){
			if(filelist[i]!=NULL){
				strcpy(filearr[used].file_type,file_types[filelist[i]->type]);
				//note : strcpy seems to be dst , src
				filearr[used].mode=filelist[i]->mode;
				filearr[used].ref_count=filelist[i]->ref_count;
				filearr[used].filepos=filelist[i]->offp;//fornow	
				used++;
			}
		}	
		//the length check i said we'd do later
		if(len<used*sizeof(struct file_info)){
			return -EINVAL;
		}
		memcpy(ubuf,filearr,used*sizeof(struct file_info));
		return used;
	}
    }
      else if(cmd==GET_PINFO_MSEG){
		//memory segments info collection
    	if(ubuf!=NULL){

		struct mm_segment *segments;
		segments=ctx->mms;
		struct mem_segment_info ans[MAX_MM_SEGS];
		int used=0;	
		for(int i=0;i<MAX_MM_SEGS;i++){
			if(&segments[i]!=NULL){//i guess this is a little superfluous ? as in segments+(size)*i also works i guess ?

				strcpy(ans[used].segname,segment_names[used]);
				ans[used].start=segments[i].start;
				ans[used].end=segments[i].end;
				ans[used].next_free=segments[i].next_free;
				int access=segments[i].access_flags;

				char perms[4]="___";
				if(access&4){
					perms[2]='X';
				}
				if(access&2){
					perms[1]='W';
				}
				if(access&1){
					perms[0]='R';
				}

				strcpy(ans[used].perm,perms);
				used++;
			}
		}	
		if(len<used*sizeof(struct mem_segment_info)){
			return -EINVAL;
		}
		memcpy(ubuf,ans,used*sizeof(struct mem_segment_info));
		return used;
	}
    }
    else if(cmd==GET_PINFO_VMA){
    	if(ubuf!=NULL){

		struct vm_area *segments;
		segments=(ctx->vm_area)->vm_next; // as per the question , the first area is dummy
		struct vm_area_info ans[10000];//check again 
			//the final memcpy instruction copies to an array anyway , so we also used an array with a large size so that we
			//don't run into any size issues
		int used=0;	
		while(segments!=NULL){
				
				ans[used].start=segments->vm_start;
				ans[used].end=segments->vm_end;
				int access=segments->access_flags;

				char perms[4]="___";
				if(access&4){
					perms[2]='X';
				}
				if(access&2){
					perms[1]='W';
				}
				if(access&1){
					perms[0]='R';
				}

				strcpy(ans[used].perm,perms);
				used++;
				segments=segments->vm_next;
		}	
		if(len<used*sizeof(struct vm_area_info)){
			return -EINVAL;
		}
		memcpy(ubuf,ans,used*sizeof(struct mem_segment_info));
		return used;
	}
    }

  return retval;    
}
