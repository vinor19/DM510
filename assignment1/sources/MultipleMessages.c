#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv) {
	char *in = "This is a stupid message.";
	char *in2 = "This is another stupid message.";
	char msg[34];
	int msglen;
	       
	/* Send a message containing 'in' */
	int failure = syscall(__NR_dm510_msgbox_put, in, strlen(in)+1);
	if(failure) printf("Syscall dm510_msgbox_put failed and returned: %d\n\n", failure);
	failure = syscall(__NR_dm510_msgbox_put, in2, strlen(in2)+1);
	if(failure) printf("Syscall dm510_msgbox_put failed and returned: %d\n\n", failure);
	/* Read a message */
	msglen = syscall(__NR_dm510_msgbox_get, msg, 34);
	if(msglen>1){
	       	printf("%s\n", msg);
		printf("Message is %d characters long\n\n", msglen);
	}else{
		printf("Syscall dm510_msgbox_get failed and returned: %d\n\n", msglen);
	}
	char msg2[50];
	msglen = syscall(__NR_dm510_msgbox_get, msg2, 50);
	if(msglen>1){
	       	printf("%s\n", msg2);
		printf("Message is %d characters long\n\n", msglen);
	}else{
		printf("Syscall dm510_msgbox_get failed and returned: %d\n\n", msglen);
	}
	return 0;
}
