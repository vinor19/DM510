#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv) {
	char msg2[50];
	int msglen = syscall(__NR_dm510_msgbox_get, msg2, 50);
	if(msglen>1){
	        printf("%s\n", msg2);
		printf("Message is %d characters long\n\n", msglen); 	
	}else{
	        printf("Syscall dm510_msgbox_get failed and returned: %d\n\n", msglen);
	}
	return 0;
}
