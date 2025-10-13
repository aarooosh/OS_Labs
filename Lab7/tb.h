#ifndef __TB_H_
#define __TB_H_

#include <types.h>
#include <context.h>

///////////////////////////////////////////////////////////////////////
///////////////////// Trace buffer functionality /////////////////////
/////////////////////////////////////////////////////////////////////
#define TRACE_BUFFER_MAX_SIZE 4096

//Trace buffer information structure
struct tb_info
{
	//Modify as per the need
	char *buf; //this is the actual buffer which will store the data
	int readp; //this stores the location to start reading from 
	int writep;//this stores the location to start writing from
	int isEmpty;//is a flag used to set if the buffer is empty
};


extern int sys_create_tb(struct exec_context *current, int mode);
#endif
