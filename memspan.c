#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<sys/mman.h>

long how_large(void *address)
{
  /*
   *  TODO: Your code
   */ 
	long a=(long)address;
	a=a>>12;
	a=a<<12;
	//page align the address ewuivalent to a-a%4096
	address=(void*)a;
	// the idea is to check for every address beyond the current one and before the current one to see
	// if it is allocated or no 
	// the way we are doing this is to try and assign it / allocate it and seeing ifd it fails ,, 
	// taking care to deallocate it in case it does
	// this is being taken careof by the MAP_FIXED_NOREPLACE flag which does not force MAP_FIXED behaviour and errors instead
	//
	// Another point to note here is that the level of granularity is at the page level
	// so each address , length etc that we move or manipulate is of size 4096 bytes 
	// Q: What abput non page aligned stuff ? No idea , i'll follow this up soon.
	long lenforward=0, lenbackward=0;
	void* addressfw=address;
	void* addressbw=address-4096;
	while(1){
	//Forward loop
		void* ptr = mmap(addressfw,4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED_NOREPLACE|MAP_PRIVATE, 0, 0);
		
		if(ptr==(void*)(-1)){
			lenforward+=4096;
			addressfw+=4096;
			munmap(ptr,4096);
		}
		else{
			break;
		}	
	}
	if(lenforward==0){
	//	printf("0");
		return 0;
	}	
	while(1){
	//Backwards loop
		void* ptr = mmap(addressbw, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED_NOREPLACE|MAP_PRIVATE, 0, 0);
		if(ptr==(void*)(-1)){
			lenbackward+=4096;
			addressbw-=4096;
			munmap(ptr,4096);
		}
		else{
			break;
		}	
	}
	
//	printf("%ld",lenforward+lenbackward);
	return lenforward+lenbackward;

}
