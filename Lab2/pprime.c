#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<sys/time.h>
#include<sys/wait.h>


extern int isPrime(long x); // Returns zero if composite.
                            // Returns one if prime. You may use this if you want. 
int find_primes(long M)
{
	//Your code goes in here
	long start=2;
	long end=M/2;
	int count = 0;
	pid_t p1=0,p2=0;
	p1=fork();
	if(p1!=0){
		start=M/2+1;
		end=M;
		p2=fork();	
		if(p2==0){
			for(long i=start;i<=end;i++){
				count += isPrime(i);
				count%256;
			}
			exit(count);	
		}
		else{
			int exit1;
			int exit2;
			int c1 = waitpid(p1,&exit1,0);
			int c2 = waitpid(p2,&exit2,0);

			return (WEXITSTATUS(exit1)+WEXITSTATUS(exit2))%256;
		}
	}
	else{
		for(long i=start;i<=end;i++){
			count += isPrime(i);
			count%256;
		}
		exit(count);
	}

	return 0;   
}
