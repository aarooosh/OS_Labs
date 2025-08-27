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
	char* str1=" openat(";
	char* str2=" close(";
	char* str3=" read(";
	char* str4=" write(";
	char* str5=" stat(";
	char* str6=" execve(";
	int c1=0,c2=0,c3=0,c4=0,c5=0,c6=0;
        fd=open(argv[1],O_RDONLY);
        int curr=0;
        int size = lseek(fd,0,SEEK_END);
        lseek(fd,0,SEEK_SET);
        if(argc != 2 || fd == -1){
                printf("Unable to execute\n");
                return -1;
        }
        char* buf = (char*)(malloc(sizeof(char)*(6+3)));
        while(curr+8<size){
                read(fd , buf ,6);
                buf[6] = '\0';
                if(!strcmp(buf,str3)){
			c3++;
		}else if(!strcmp(buf,str5)){
			c5++;
		}
		read(fd , buf+6 , 1);
                buf[7] = '\0';
                if(!strcmp(buf,str2)){
			c2++;
		}else if(!strcmp(buf,str4)){
			c4++;
		}
		read(fd , buf+7 , 1);
                buf[8] = '\0';
                if(!strcmp(buf,str1)){
			c1++;
		}else if(!strcmp(buf,str6)){
			c6++;
		}




                lseek(fd,-7,SEEK_CUR);
                curr++;
        }
        printf("openat: %d\nclose: %d\nread: %d\nwrite: %d\nstat: %d\nexecve: %d\n",c1,c2,c3,c4,c5,c6);
        return 0;
}

