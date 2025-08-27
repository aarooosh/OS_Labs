#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<sys/time.h>
#include<sys/wait.h>

int main(int argc, char **argv)
{
   /*Your code goes here */
	int ans=0;
	if(argc==2){
		 ans=1;
	}
	else{	
		ans=atoi(argv[2]);
	}
	int p=atoi(argv[1]);
	if(p==2){
		printf("%d\n", ans*p);
	}
	else{
		char* buf1=(char*)malloc(sizeof(char)*100);	
		char* buf2=(char*)malloc(sizeof(char)*100);
		sprintf(buf1,"%d", p-1);
		sprintf(buf2,"%d", ans*p);
			
		execl("./fact", "fact", buf1,buf2,NULL);

		
	}
	return 0;
}
