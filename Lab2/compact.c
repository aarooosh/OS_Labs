#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

void compact(void *start, void *end)
{
  /* 
   * TODO
   * Your code goes here
   */
	
	char* p1 = (char*)start;
	char* p2 = (char*)start;

	while(p2 != (char*)end)
	{
		if(*p2 != '\0'){
			*p1 = *p2;
			p1++;
		}
		p2++;
	}
	sbrk((int)(p1-p2));
  return ;    
}
