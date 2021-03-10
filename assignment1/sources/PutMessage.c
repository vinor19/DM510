#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv) {
	char *in = "This is a stupid message.";
	int msglen;
	       
	/* Send a message containing 'in' */
	int failure = syscall(__NR_dm510_msgbox_put, in, strlen(in)+1);
	if(failure) printf("Syscall dm510_msgbox_put failed and returned: %d\n\n", failure);

	return 0;
}
