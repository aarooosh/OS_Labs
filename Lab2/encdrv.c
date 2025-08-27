#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<assert.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>

#define MAX_CHARS_IN_LINE 1000

int main(int argc, char **argv)
{
	char buf[MAX_CHARS_IN_LINE+1];
	char obuf[MAX_CHARS_IN_LINE+1];

	// first i'll pipe, then set up a fork, then do dup2 (happens inside child  for i/o) , then do execl
	//
	
	char* ifilename = argv[1];			// input file
	char* ofilename = argv[2];			// output file
	int fdp[2]; 					// fd for pipe
	int fdi = open(ifilename , O_RDONLY); 		// opening both files
	int fdo = open(ofilename , O_RDWR | O_CREAT, 0666);//O_cREArt if doesnt excist
	assert(fdi > 0);
	assert(fdo > 0);
// set up a pipe
		pipe(fdp);				//set up a pipe
		int pid = fork();			//create a fork
		if(pid==0){
							//child seeing this
			close(0);			//closing stdin for child
			close(1);			//close stdout for child
			dup(fdp[0]); 			//dup and replace stdin with fdp[0] , i.e. read from the pipe
			dup(fdo);			//dup and replace strdout with fdo , write to ooutput file
			close(fdp[1]);			//close write end of pipe from child
			// now stdin for the child is the read input for the pipe
			// now stdout for the child is the output file 
			execl("./encrypt","encrypt",NULL); // it'll read from stdin
			return 0;
		}
		else{	//parent is seeing this
			//
			
			char* str = (char*)(malloc(1000));//sprintf takes by value so yoiu have to allocate the moery beforehans 
			char* line = (char*)(malloc(sizeof(char)*(MAX_CHARS_IN_LINE+1))); // namesake buf for readto prevent s4egfault , it can resize
			close(fdp[0]);			//closing read end of the pipe fro parent
			int bytes;			// bytes read
			int linel;			//length of the line
			while(1){
				bytes = read(fdi,line,MAX_CHARS_IN_LINE + 1);// read a big chunk at a time and find in line
				if(bytes == 0){		//close and kill because of EOF reached
				close(fdp[1]);		//close write of piep from parent
				close(fdo);		//close output file descripor (in case the child encrypt still thinks tht there is some refernce to that and keeps it open )
				close(fdi);	
					return 0;
				}
				else{ 			// loop over line to find \n 
					line[bytes]= '\0'; // null terminate for strlen
					for(int i = 0; i < strlen(line); i++){//loop over this new length
						if(line[i] == '\n'){
							//if end of line detected
							line[i+1] = '\0';
							lseek(fdi,-bytes+i+1,1);
							//set file pointer back to next line
							linel = i+1;
							break;
						}
					}
					int chars = sprintf(str,"%d\n",linel);//cast the value of linel to a string
					strcat(str,line);// extremely shitty doofus dumbo mistake
					write(fdp[1],str,chars);
					write(fdp[1],line,linel);
							//writing each line read by parent to the pipe
				}
		
		}}
	

	
// NOTE: Do not modify anything above this line	
/***
 *      Your Code goes here
 */	

	return 0;
}

