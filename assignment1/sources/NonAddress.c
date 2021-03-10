#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv) {
	int failure = syscall(__NR_dm510_msgbox_put, 20, 40);
	if(failure) printf("Syscall dm510_msgbox_put failed and returned: %d\n\n", failure);	
	/* Read a message */
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
