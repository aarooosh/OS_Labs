#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char **argv) {

    /**
     * TODO: Write your code here.
     */
	
	int fd;
	fd=open(argv[2],O_RDONLY);
	int curr=0;
	int len=strlen(argv[1]);
	int size = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	if(argc != 3 || fd == -1){
		printf("Error\n");
		return -1;
	}
	char* buf = (char*)(malloc(sizeof(char)*(len+1)));
	while(curr+len<size){
		read(fd , buf , len);
		buf[len] = '\0';
		if(!strcmp(buf,argv[1])){
		printf("FOUND\n");
		return 0;
		}
		lseek(fd,-len+1,SEEK_CUR);
		curr++;
	}
	printf("NOT FOUND\n");
	return 0;
}
