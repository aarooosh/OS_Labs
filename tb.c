#include<tb.h>
#include<lib.h>
#include<file.h>
#include<context.h>
#include<memory.h>

/*
 * *  Trace buffer implementation.
 *
 */


///////////////////////////////////////////////////////////////////////////
////           TODO:     Trace buffer functionality                   /////
///////////////////////////////////////////////////////////////////////////


// Check whether passed buffer is valid memory location for read.
static int tb_validate(unsigned long buff, u32 count, int acflags) 
{
	int ifX=acflags/8;
	int ifW=(acflags/4)%2;
	int ifR=acflags%2;
	struct exec_context *ctx=get_current_ctx();
	for(int i=0;i<MAX_MM_SEGS-1;i++){
		if(buff+count<ctx->mms[i].next_free && buff>= ctx->mms[i].start){
		//	if((acflags/4 <= ctx->mms[i].access_flags/4)&&((acflags/2)%2 <= (ctx->mms[i].access_flags/2)%2) && (acflags%2 <= ctx->mms[i].access_flags%2)){
			if((acflags==1&&(ctx->mms[i].access_flags%2==1))||(acflags==2&&(ctx->mms[i].access_flags/2)%2==1)){
				return 1;
			}
			else{
				return 0;
			}
		}
	}
	if(buff+count<ctx->mms[3].end && buff>= ctx->mms[3].start){
		//	if((acflags/4 <= ctx->mms[i].access_flags/4)&&((acflags/2)%2 <= (ctx->mms[i].access_flags/2)%2) && (acflags%2 <= ctx->mms[i].access_flags%2)){
			if((acflags==1&&(ctx->mms[3].access_flags%2==1))||(acflags==2&&(ctx->mms[3].access_flags/2)%2==1)){
				return 1;
			}
			else{
				return 0;
			}
		}
	struct vm_area* head=ctx->vm_area;
	while(head!=NULL){
		if(buff+count<head->vm_end && buff>= head->vm_start){
			if((acflags==1&&(head->access_flags%2==1))||(acflags==2&&(head->access_flags/2)%2==1)){
				return 1;
			}
			else{
				return 0;
			}
		}
		head=head->vm_next;
	}
    //  printk("Validate Error\n");	
      return 0;
}

static long tb_close(struct file *filep)
{
	if(filep->tb->buf==NULL||filep->tb==NULL||filep->fops==NULL||filep==NULL){
		return -EINVAL;
	}
	os_page_free(USER_REG, filep->tb->buf);
	os_free(filep->tb,sizeof(struct tb_info));
	os_free(filep->fops,sizeof(struct fileops));
	os_free(filep, sizeof(struct file));
	//prolly not needed
	//struct exec_context *ctx=get_current_ctx();
        //printk("close Error\n");	
        return 0;
}

static int tb_read(struct file *filep, char *buff, u32 count)
{
	if(filep->tb->buf==NULL||filep->tb==NULL||filep->fops==NULL||filep==NULL){
		return -EINVAL;
	}
	if(count==0){return 0;}	
	if(!tb_validate((unsigned long)buff, count, 2)){return -EBADMEM;}
	int R=filep->tb->readp;
	int W=filep->tb->writep;
	char* currbuf=filep->tb->buf;
	int filled=0;
	if(filep->tb->isEmpty){
		return 0;
	}
	int yikes = 0;
	while(filled<count){
		if(R==W && yikes){	filep->tb->readp=R;
			filep->tb->writep=W;
			filep->tb->isEmpty = 1;
  // printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 

			return filled;
		}
		if(R<TRACE_BUFFER_MAX_SIZE-1){
			yikes = 1;
			buff[filled]=currbuf[R];
			filled++;
			R++;
			if(R==W && yikes){	filep->tb->readp=R;
				filep->tb->writep=W;
				filep->tb->isEmpty = 1;
//				printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
				return filled;
		}

		}
		else{
			yikes=1;
			buff[filled]=currbuf[R];
			filled++;
			R=0;
			if(W==0){
				filep->tb->readp=0;
				filep->tb->writep=W;
  // printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
				if(R==W && yikes){	
					filep->tb->readp=R;
					filep->tb->writep=W;
					filep->tb->isEmpty = 1;
//				printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
					return filled;
				}

				return filled;	
			}
		}
	}
		filep->tb->readp=R;
			filep->tb->writep=W;
   //printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
			return filled;

//	printk("read Error\n");	
 //   return -1;

}

static int tb_write(struct file *filep, char *buff, u32 count)
{
	if(filep->tb->buf==NULL||filep->tb==NULL||filep->fops==NULL||filep==NULL){
		return -EINVAL;
	}
	
	if(count==0)
		{
//			printk("isEmpty %d\n", filep->tb->isEmpty);
		return 0;}	

	if(!tb_validate((unsigned long)buff, count, 1)){return -EBADMEM;}
	int R=filep->tb->readp;
	int W=filep->tb->writep;
	char* currbuf=filep->tb->buf;
	int filled=0;
	while(filled<count){
		if(R==W&&!filep->tb->isEmpty){
			filep->tb->readp=R;
			filep->tb->writep=W;
			filep->tb->isEmpty = 0;
  // printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
			return filled;
		}
		if(W<TRACE_BUFFER_MAX_SIZE-1){
			currbuf[W]=buff[filled];
			filled++;
			W++;
			filep->tb->isEmpty = 0;
		}
		else{
			currbuf[W]=buff[filled];
			filled++;
			if(R==0){
				filep->tb->readp=R;
				filep->tb->writep=0;
				filep->tb->isEmpty = 0;
   //printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
				return filled;	
			}
			W=0;
		}
	}
		filep->tb->readp=R;
			filep->tb->writep=W;
  // printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
			return filled;
   // printk("write Error\n");	
   // return -1;
}

int sys_create_tb(struct exec_context *current, int mode)
{
	struct file** curr_files=current->files;
	int first_free=0;
	for(int i=0;i<MAX_OPEN_FILES+1;i++){
		if(i==MAX_OPEN_FILES){
			return -EINVAL;
		}
		if(curr_files[i]==NULL){
			first_free=i;
			break;	
		}
	}

	struct file *tb_file=os_alloc(sizeof(struct file));
	if(tb_file==(void*)-1){return -ENOMEM;}

	curr_files[first_free]=tb_file;
	tb_file->type=TRACE_BUFFER;
	tb_file->mode=mode;
	tb_file->offp=0;
	tb_file->ref_count=1;
	tb_file->inode=NULL;
	tb_file->pipe=NULL;
	//create tb_info, and point to our tb;
	struct tb_info *curr_tb=os_alloc(sizeof(struct tb_info));
	if(curr_tb==(void*)-1){return -ENOMEM;}
	curr_tb->readp=0;
	curr_tb->writep=0;
	curr_tb->isEmpty=1;
	curr_tb->buf=(char*)os_page_alloc(USER_REG);

	//fileops bhi karo figure out
	struct fileops *curr_tb_ops=os_alloc(sizeof(struct fileops));
	if(curr_tb_ops==(void*)-1){return -ENOMEM;}

	curr_tb_ops->read=tb_read;
	curr_tb_ops->write=tb_write;
	curr_tb_ops->close=tb_close;
	tb_file->fops = curr_tb_ops;
	tb_file->tb = curr_tb;

//	int ret_fd = -1;
	//printk("first_free=%d\n", first_free);	
	return first_free;
}
