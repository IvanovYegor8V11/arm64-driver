#ifndef CIPHERDEV_IOCTL_H
#define CIPHERDEV_IOCTL_H
#include <linux/ioctl.h>
#include <linux/types.h>
#define CIPHERDEV_IOC_MAGIC 'C'
#define CIPHERDEV_SET_KEY      _IOW(CIPHERDEV_IOC_MAGIC, 1, __u32)
#define CIPHERDEV_SET_MODE     _IOW(CIPHERDEV_IOC_MAGIC, 2, __u32)
#define CIPHERDEV_GET_STATS    _IOR(CIPHERDEV_IOC_MAGIC, 3, struct cipherdev_stats)
#define CIPHERDEV_RESET        _IO(CIPHERDEV_IOC_MAGIC, 4)
#define CIPHERDEV_GET_CHECKSUM _IOR(CIPHERDEV_IOC_MAGIC, 5, __u32)
struct cipherdev_stats { __u64 writes; __u64 reads; __u64 bytes_in; __u64 bytes_out; };
#endif
