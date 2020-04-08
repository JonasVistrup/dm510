#include "linux/kernel.h"
#include "linux/unistd.h"
#include "linux/slab.h"
#include "linux/uaccess.h"

/*
	Authors: Jonas Vistrup, 	jovis18
		 Mikkel Hejsel, 	mihej18
		 Thomas Lindal Winther,	thwin18
*/

typedef struct _msg_t msg_t;

//Stack of messages
struct _msg_t{
	msg_t* previous;
	int length;
	char* message;
};

static msg_t* top;


//Function, which puts a message onto the stack, where the message is of length "length" and in char* "buffer"
asmlinkage
int sys_dm510_msgbox_put(char *buffer, int length){
	void* ptr;
	msg_t* msg;
	unsigned long flags;

	if(length<0){
		return -EINVAL;	//cant copy negative length
	}

	if(access_ok(buffer,length)==0){
                return -ENOBUFS; //Buffer space not available
        }

	ptr = kmalloc(sizeof(msg_t), GFP_KERNEL);
	if(!ptr){
		return -ENOMEM;	//if malloc fails, malloc return a null pointer
	}


	msg = ptr;
	msg->previous = NULL;
	msg->length = length;
	ptr = kmalloc(length, GFP_KERNEL);
	if(!ptr){
        	kfree(msg);
		return -ENOMEM;	//if malloc fails, malloc returns a null pointer
	}
	msg->message = ptr;


	if(copy_from_user(msg->message, buffer, length)!=0){
		kfree(msg->message);
        	kfree(msg);
		return -EIO;	//If the copy could not copy all the bytes, it will return the number of failed bytes, on succes return 0
	}

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

//Function, which pops the first element of the stack and copies its message into the "buffer", iff the buffer is big enogh to contain the message, as indicated by the "length" parameter..
asmlinkage
int sys_dm510_msgbox_get(char *buffer, int length){
	unsigned long flags;
	msg_t* msg;
	int mlength;

	local_irq_save(flags);
	if(top==NULL){
		local_irq_restore(flags);
		return -ENODATA;
	}
	msg = top;
	mlength = msg->length;

	if(mlength>length){
		local_irq_restore(flags);
		return -ENOMEM;		//Not enough space
	}

	if(access_ok(buffer, mlength)==0){
		local_irq_restore(flags);
                return -ENOBUFS;                //Buffer space not available
        }

	if(copy_to_user(buffer, msg->message, mlength)!=0){
		local_irq_restore(flags);
		return -EIO;
	}

	top = msg->previous;

        local_irq_restore(flags);

	/* free memory */
	kfree(msg->message);
	kfree(msg);
	return mlength;
}
