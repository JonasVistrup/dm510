#ifndef DM510_DEV_H
#define DM510_DEV_H

#include <linux/ioctl.h>

#define DEVICE_NAME "dm510_dev" /* Dev name as it appears in /proc/devices */
#define MAJOR_NUMBER 254
#define MIN_MINOR_NUMBER 0
#define MAX_MINOR_NUMBER 1

#define DEVICE_COUNT 2

#define IOC_MAGIC 'k'

#define IOC_B_READ_SIZE		_IOW(IOC_MAGIC, 0, int)
#define IOC_B_WRITE_SIZE 	_IOW(IOC_MAGIC, 1, int)
#define IOC_NUM_OF_READERS 	_IOW(IOC_MAGIC, 2, int)

#endif
