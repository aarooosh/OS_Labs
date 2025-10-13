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

//WARNING:We've implemented it very messily , i'm sure there's a cleaner way to go about doing it and dealing with the edge cases


// Check whether passed buffer is valid memory location for read or write.
static int tb_validate(unsigned long buff, u32 count, int acflags) 
{
	// int ifX=acflags/8;
	// int ifW=(acflags/4)%2;
	// int ifR=acflags%2;

	//acflags is passed as an XWR (ikik it's weirddd sorryyy)
	
	struct exec_context *ctx=get_current_ctx();

	//loop over code segment , data segment and heap to see if the memory address lies COMPLETELY within one of these blocks
	//note : we initially used segment.end instead of next free page which is where we got errors
	for(int i=0;i<MAX_MM_SEGS-1;i++){
		if(buff+count<ctx->mms[i].next_free && buff>= ctx->mms[i].start){
		//if((acflags/4 <= ctx->mms[i].access_flags/4)&&((acflags/2)%2 <= (ctx->mms[i].access_flags/2)%2) && (acflags%2 <= ctx->mms[i].access_flags%2)){
			
			//checking the matching permissions
			if((acflags==1&&(ctx->mms[i].access_flags%2==1))||(acflags==2&&(ctx->mms[i].access_flags/2)%2==1)){
				return 1;
			}
			else{
				return 0;
			}
		}
	}
	//for stack segment we have to check stack.end and stack.start ! 
	if(buff+count<ctx->mms[3].end && buff>= ctx->mms[3].start){
		//	if((acflags/4 <= ctx->mms[i].access_flags/4)&&((acflags/2)%2 <= (ctx->mms[i].access_flags/2)%2) && (acflags%2 <= ctx->mms[i].access_flags%2)){
			if((acflags==1&&(ctx->mms[3].access_flags%2==1))||(acflags==2&&(ctx->mms[3].access_flags/2)%2==1)){
				return 1;
			}
			else{
				return 0;
			}
		}

	//if not in any of the memory segments , check if it lies in allocated virtual memory areas associated with this process
	//vm_area is stored as a linked list of type vm_area
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
	os_page_free(USER_REG, filep->tb->buf); // this thankfully worked but i think the best way would've been the line below :
	//os_page_free(USER_REG, ((filep->tb->buf)>>3)<<3); this would give us the start of the page address
	//it worked here because the size of the buffer was 4096 bytes i.e. one page and all allocations would be page aligned
	os_free(filep->tb,sizeof(struct tb_info));
	os_free(filep->fops,sizeof(struct fileops));
	os_free(filep, sizeof(struct file));
	
	//struct exec_context *ctx=get_current_ctx();
    //printk("close Error\n");	
    return 0;
}

static int tb_read(struct file *filep, char *buff, u32 count)
{
	//quick check if any of the necessary fields aren't allocated , to error out
	if(filep->tb->buf==NULL||filep->tb==NULL||filep->fops==NULL||filep==NULL){
		return -EINVAL;
	}
	//if count is zero return bytes read as zero and don't do anything
	if(count==0){return 0;}	
	//validate if the buffer we're trying to write to is legal to do or not
	if(!tb_validate((unsigned long)buff, count, 2)){return -EBADMEM;}
	
	int R=filep->tb->readp;
	int W=filep->tb->writep;
	char* currbuf=filep->tb->buf;
	int filled=0;
	if(filep->tb->isEmpty){
		return 0;//cannot read from an empty buffer
	}
	int yikes = 0;
	//yikes is a flag which will help check if during the course of a read the read and write pointer
	//match up , indicating that it's become empty (and will allow us to set the isEmpty flag)
	//yikes will be set once at least one byte is read , as it's possible for R==W even before we start reading
	//if R==W before anything is read , it means it's full ! 
	//if R==W during the course of a read , it means it's empty !
	while(filled<count){
		if(R==W && yikes){	
			filep->tb->readp=R;
			filep->tb->writep=W;
			filep->tb->isEmpty = 1;
			//debugging printf			  
  			//printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty);
			return filled;
		}
		if(R<TRACE_BUFFER_MAX_SIZE-1){
			yikes = 1;
			buff[filled]=currbuf[R];
			filled++;
			R++;
			//had to copy this check again because we're doing filled<count
			//so it's possible that it becomes empty after the last byte is read and if not for the if below
			//we would've missed it
			if(R==W && yikes){	
				filep->tb->readp=R;
				filep->tb->writep=W;
				filep->tb->isEmpty = 1;
				//debugging printf
				//printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
				return filled;
			}

		}
		else{
			//entering this block means it's reached the end during the course of a read
			//so the read pointer would have to wrap back around
			yikes=1; //remember ! yikes is set to 1 everytime something is read (to help with the empty checks)
			buff[filled]=currbuf[R];
			filled++;
			R=0;
			if(W==0){
				//some of this is superflous 
				//W==0 means that our read is to be terminated 
				//again this needs to be specifically checked because of the filled<count thing last byte read etc.
				filep->tb->readp=0;
				filep->tb->writep=W;
				//debugging printf
  				// printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
				if(R==W && yikes){	
					filep->tb->readp=R;
					filep->tb->writep=W;
					filep->tb->isEmpty = 1;
					//debugging printf
					//printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
					return filled;
				}

				return filled;	
			}
		}
	}
	filep->tb->readp=R;
	filep->tb->writep=W;
	//debugigng printf			
   	//printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
	return filled;
	//printk("read Error\n");	
 	//return -1;

}

static int tb_write(struct file *filep, char *buff, u32 count)
{
	//similar edge case handling holds for write as was mentioned in read
	if(filep->tb->buf==NULL||filep->tb==NULL||filep->fops==NULL||filep==NULL){
		return -EINVAL;
	}
	
	if(count==0)
		{
		//printk("isEmpty %d\n", filep->tb->isEmpty);
		return 0;
		}	

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
			//the moment anything is written , make is empty to be false
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
    //printk("r w isEmpty = %d %d %d\n", R, W, filep->tb->isEmpty); 
	return filled;
    //printk("write Error\n");	
    //return -1;
}

int sys_create_tb(struct exec_context *current, int mode)
{
	//current [PCB] stores files which is an array of file pointers
	//such that current[fd] = file pointer to the file associated with the FD
	struct file** curr_files=current->files;
	int first_free=0; //will be used to store the first free file descriptor
	for(int i=0;i<MAX_OPEN_FILES+1;i++){
		if(i==MAX_OPEN_FILES){
			return -EINVAL;
			//return error if we've reached the limit of max open files for a process
		}
		if(curr_files[i]==NULL){
			first_free=i;
			break;	
		}
	}

	//create a file object whose fields will be set such that it's of type trace buffer 
	//we will copy this into the PCB ad set all the pointers accordingly
	struct file *tb_file=os_alloc(sizeof(struct file));
	if(tb_file==(void*)-1){return -ENOMEM;} //error out if no memory space left to allocate 

	curr_files[first_free]=tb_file;
	//remember ! tb_file is a file object !
	tb_file->type=TRACE_BUFFER;
	tb_file->mode=mode;
	tb_file->offp=0;
	tb_file->ref_count=1;//will be important for future application if fork is called and when to delete object
	tb_file->inode=NULL;
	tb_file->pipe=NULL;
	//inode and pipes are set to NULL because we want the file object to act like a trace buffer
	//create tb_info, and point to our tb;
	struct tb_info *curr_tb=os_alloc(sizeof(struct tb_info));
	if(curr_tb==(void*)-1){return -ENOMEM;}
	curr_tb->readp=0;
	curr_tb->writep=0;
	curr_tb->isEmpty=1;
	curr_tb->buf=(char*)os_page_alloc(USER_REG);//not sure what USER_REG is exactly , maybe it's User memory region (since this is being called for a user process? Not sure)
	
	struct fileops *curr_tb_ops=os_alloc(sizeof(struct fileops));
	if(curr_tb_ops==(void*)-1){return -ENOMEM;}
	//the idea is that the syscall will blindly transfer control to the functions pointed in the file object's fops field
	//when a read/write/close etc is called on any file object
	//so we have to point the fops fields to our tb_read/write/close functions so that it manipulates this trace buffer
	curr_tb_ops->read=tb_read;
	curr_tb_ops->write=tb_write;
	curr_tb_ops->close=tb_close;
	tb_file->fops = curr_tb_ops;
	tb_file->tb = curr_tb;

	//debugging printfs below
	//int ret_fd = -1;
	//printk("first_free=%d\n", first_free);	
	return first_free;
}
