#include<procinfo.h>
#include<lib.h>
#include<file.h>
#include<context.h>

static char* segment_names[MAX_MM_SEGS+1] = {"code", "rodata", "data", "stack", "invalid"}; 
static char* file_types[MAX_FILE_TYPE+1] = {"stdin", "stdout", "stderr", "reg", "pipe", "invalid"}; 

long get_process_info(struct exec_context *ctx, long cmd, char *ubuf, long len)
{
    long retval = -EINVAL;	
  /*
     * TODO your code goes in here
     * */

    //printk("%d\n",sizeof(struct general_info));
    //printk("%d\n",len >= sizeof(struct general_info));
   // printk("%d\n",cmd == GET_PINFO_GEN);
    if(cmd==GET_PINFO_GEN){
    	if(ubuf!=NULL && len >=sizeof(struct general_info)){

		//printk("%d\n",cmd == GET_PINFO_GEN);
		//assuming we can cast ubuf directly because its already allocated and has enough space
		
		struct general_info casted_ubuf; 
		//struct general_info* casted_ubuf= (struct general_info*)ubuf; 
		casted_ubuf.pid=ctx->pid;
		casted_ubuf.ppid=ctx->ppid;
		casted_ubuf.pcb_addr=(unsigned long)ctx;
		strcpy(casted_ubuf.pname, ctx->name);
		memcpy(ubuf,&casted_ubuf,sizeof(struct general_info));
		//casted_ubuf->pname=ctx->name;
		retval=1;
		//printk("%d\n",retval);
	}
    }
     else if(cmd==GET_PINFO_FILE){
    	if(ubuf!=NULL){

		//printk("%d\n",cmd == GET_PINFO_GEN);
		//assuming we can cast ubuf directly because its already allocated and has enough space
		struct file** filelist=ctx->files;
		struct file_info filearr[MAX_OPEN_FILES];
		int used=0;
		for(int i=0;i<MAX_OPEN_FILES;i++){
			if(filelist[i]!=NULL){
				strcpy(filearr[used].file_type,file_types[filelist[i]->type]);
				filearr[used].mode=filelist[i]->mode;
				filearr[used].ref_count=filelist[i]->ref_count;
				filearr[used].filepos=filelist[i]->offp;//fornow	
				used++;
			}
		}	
		if(len<used*sizeof(struct file_info)){
			return -EINVAL;
		}
		memcpy(ubuf,filearr,used*sizeof(struct file_info));
		return used;
	}
    }
      else if(cmd==GET_PINFO_MSEG){
    	if(ubuf!=NULL){

		struct mm_segment *segments;
		segments=ctx->mms;
		struct mem_segment_info ans[MAX_MM_SEGS];
		int used=0;	
		for(int i=0;i<MAX_MM_SEGS;i++){
			if(&segments[i]!=NULL){

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
		segments=(ctx->vm_area)->vm_next;
		struct vm_area_info ans[10000];//check again
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
