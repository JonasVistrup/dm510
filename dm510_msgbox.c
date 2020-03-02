#include "linux/kernel.h"
#include "linux/unistd.h"
#include "linux/slab.h"
#include "linux/uaccess.h"

typedef struct _msg_t msg_t;

struct _msg_t{
	msg* previous;
	int length;
	char* message;
};

static msg_t* top;


asmlinkage
int sys_dm510_msgbox_put(char *buffer, int length){
	if(length<0){
		return -1;	//cant copy negative length
	}
	void *ptr = kmalloc(sizeof(msg_t));
	if(!ptr){
		return -1;	//if malloc fails, malloc return a null pointer
	}
	msg_t* msg = ptr;
	msg->previous = NULL;
	msg->length = length;
	void *ptr = kmalloc(length);
	if(!ptr){
		return -1;	//if malloc fails, malloc returns a null pointer
	}
	msg->message = ptr;
	if(access_ok(buffer,length)==0){
		return -1;	//If data could not be read or written access_ok returns 0
	}
	if(copy_from_user(msg->message, buffer, length)!=0){
		return -1;	//If the copy could not copy all the bytes, it will return the number of failed bytes, on succes return 0
	}

	unsigned long flags;
	local_irq_save(flags);
	if(top==NULL){
		top = msg;
	}else{
		msg->previous = top;
		top = msg;
	}
	local_irq_restore(flags);
	return 0;
}

asmlinkage
int sys_dm510_msgbox_get(char *buffer, int length){
	unsigned long flags;
	local_irq_save(flags);
	if(top==NULL){
		local_irq_restore(flags);
		return -1;
	}
	msg_t* msg = top;
	int mlength = msg->length;
	top = msg->previous;

	local_irq_restore(flags);

	if(mlength<length){
		return -1;
	}

	if(access_ok(buffer, mlength)==0){
		return -1;
	}
	if(copy_to_user(buffer, msg->message, mlength)!=0){
		return -1;
	}


	/* free memory */
	kfree(msg->message);
	kfree(msg);

	return mlength;
}
