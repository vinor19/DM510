#include "linux/kernel.h"
#include "linux/unistd.h"
#include "linux/slab.h"
#include "linux/uaccess.h"

typedef struct _msg_t msg_t;

struct _msg_t{
	msg_t* previous;
	int length;
	char* message;
};

static msg_t *bottom = NULL;
static msg_t *top = NULL;

asmlinkage
int sys_dm510_msgbox_put( char *buffer, int length ) {
	if(!access_ok(buffer, length)) {
		return -EFAULT;
	}else{
		unsigned long flags;
		msg_t* msg = kmalloc(sizeof(msg_t), GFP_KERNEL);
	
		if(msg == NULL)	return -EINVAL;
		
		local_irq_save(flags);
		msg->previous = NULL;
		msg->length = length;
		msg->message = kmalloc(length, GFP_KERNEL);
	
		if(msg->message == NULL) return -EINVAL;
	
		if(copy_from_user(msg->message, buffer, length)) return -EFAULT;
	
		if (bottom == NULL) {
			bottom = msg;
			top = msg;
		} else {
			/* not empty stack */
			msg->previous = top;
			top = msg;
		}
		local_irq_restore(flags);
		return 0;
	}
}

asmlinkage
int sys_dm510_msgbox_get( char* buffer, int length ) {
	if (top != NULL) {
		unsigned long flags;
		msg_t* msg = top;
		int mlength = msg->length;
		if (length < mlength) return -EINVAL; //Checks if the buffer is acceptable for our message
			if (!access_ok(buffer, mlength)) return -EFAULT;
			top = msg->previous;
			local_irq_save(flags);
			/* copy message */
			if(copy_to_user(buffer, msg->message, mlength)) return -EFAULT;
			
			/* free memory */
			kfree(msg->message);
			kfree(msg);
			
			local_irq_restore(flags);

			return mlength;
	}
	return -ENOMSG;
}
