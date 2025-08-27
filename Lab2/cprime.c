#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<sys/syscall.h>
#include<string.h>

extern int isPrime(long x); // Returns zero if composite.
                            // Returns one if prime. 

long find_primes(int num_cp, long M)
{
   /*
    *  TODO 
    *  Your code goes here
    */	
	int pid;
	int fd[num_cp][2];
	int start=1;
	int interval=(M/num_cp)+1;
	for(int i=0;i<num_cp;i++){
		start=i*interval+1;
		//pipe(fd[i]);
		syscall(SYS_pipe,fd[i]);
		pid=fork();
		if(pid!=0){
			// parent
		}
		else
		{
		// child
			int count=0;
			for(int j=start;j<start+interval;j++){
				if(j>M){
					break;
				}
				count+=isPrime(j);
			}
			char  buf[64];
			sprintf(buf,"%d",count);
			write(fd[i][1],buf,64);
			
			return 0;
		}
	}
	if(pid!=0){
		int countx=0;
		for(int i=0;i<num_cp;i++){
			char buf[64];
			read(fd[i][0],buf,64);
			countx+=atoi(buf);
		}

		return countx;   
	}	

}
