#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include "cipherdev_ioctl.h"
#define BUF_CAP 4096
struct cipher_session {
    struct mutex lock;
    u8 buffer[BUF_CAP];
    size_t length;
    u8 key;
    u32 mode;
    u32 checksum;
    struct cipherdev_stats stats;
};
static u32 checksum32(const u8 *p, size_t n)
{
    u32 hash = 2166136261U;
    size_t i;
    for (i = 0; i < n; ++i) { hash ^= p[i]; hash *= 16777619U; }
    return hash;
}
static int cipher_open(struct inode *inode, struct file *file)
{
    struct cipher_session *s = kzalloc(sizeof(*s), GFP_KERNEL);
    if (!s) return -ENOMEM;
    mutex_init(&s->lock);
    s->key = 0x5a;
    file->private_data = s;
    pr_info("cipherdev: opened pid=%d\n", task_pid_nr(current));
    return 0;
}
static int cipher_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    pr_info("cipherdev: closed\n");
    return 0;
}
static ssize_t cipher_write(struct file *file, const char __user *src,
                            size_t count, loff_t *pos)
{
    struct cipher_session *s = file->private_data;
    u8 *input;
    size_t i, n;
    u8 prev = 0;
    if (!count) return 0;
    n = min_t(size_t, count, BUF_CAP);
    input = memdup_user(src, n);
    if (IS_ERR(input)) return PTR_ERR(input);
    if (mutex_lock_interruptible(&s->lock)) { kfree(input); return -ERESTARTSYS; }
    for (i = 0; i < n; ++i) {
        u8 x = input[i];
        switch (s->mode) {
        case 0: s->buffer[i] = x ^ s->key; break;
        case 1: {
            unsigned int shift = s->key & 7;
            s->buffer[i] = shift ? (u8)((x << shift) | (x >> (8 - shift))) : x;
            break;
        }
        default:
            prev = (u8)(x + s->key + prev);
            s->buffer[i] = prev;
            break;
        }
    }
    s->length = n;
    s->checksum = checksum32(s->buffer, n);
    s->stats.writes++;
    s->stats.bytes_in += n;
    *pos = 0;
    pr_info("cipherdev: processed %zu bytes mode=%u checksum=%08x\n", n, s->mode, s->checksum);
    mutex_unlock(&s->lock);
    kfree(input);
    return n;
}
static ssize_t cipher_read(struct file *file, char __user *dst,
                           size_t count, loff_t *pos)
{
    struct cipher_session *s = file->private_data;
    ssize_t n;
    if (mutex_lock_interruptible(&s->lock)) return -ERESTARTSYS;
    n = simple_read_from_buffer(dst, count, pos, s->buffer, s->length);
    if (n > 0) { s->stats.reads++; s->stats.bytes_out += n; }
    pr_info("cipherdev: read result=%zd\n", n);
    mutex_unlock(&s->lock);
    return n;
}
static long cipher_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct cipher_session *s = file->private_data;
    u32 value;
    long ret = 0;
    if (_IOC_TYPE(cmd) != CIPHERDEV_IOC_MAGIC) return -ENOTTY;
    if (mutex_lock_interruptible(&s->lock)) return -ERESTARTSYS;
    switch (cmd) {
    case CIPHERDEV_SET_KEY:
        if (copy_from_user(&value, (void __user *)arg, sizeof(value))) { ret = -EFAULT; break; }
        if (value > 255) { ret = -EINVAL; break; }
        s->key = (u8)value;
        pr_info("cipherdev: key updated\n");
        break;
    case CIPHERDEV_SET_MODE:
        if (copy_from_user(&value, (void __user *)arg, sizeof(value))) { ret = -EFAULT; break; }
        if (value > 2) { ret = -EINVAL; break; }
        s->mode = value;
        pr_info("cipherdev: mode=%u\n", value);
        break;
    case CIPHERDEV_GET_STATS:
        if (copy_to_user((void __user *)arg, &s->stats, sizeof(s->stats))) ret = -EFAULT;
        else pr_info("cipherdev: stats requested\n");
        break;
    case CIPHERDEV_RESET:
        memset(s->buffer, 0, sizeof(s->buffer));
        s->length = 0;
        s->checksum = 0;
        s->key = 0x5a;
        s->mode = 0;
        memset(&s->stats, 0, sizeof(s->stats));
        file->f_pos = 0;
        pr_info("cipherdev: session reset\n");
        break;
    case CIPHERDEV_GET_CHECKSUM:
        value = s->checksum;
        if (copy_to_user((void __user *)arg, &value, sizeof(value))) ret = -EFAULT;
        else pr_info("cipherdev: checksum requested\n");
        break;
    default: ret = -ENOTTY;
    }
    mutex_unlock(&s->lock);
    return ret;
}
static const struct file_operations cipher_fops = {
    .owner = THIS_MODULE, .open = cipher_open, .release = cipher_release,
    .read = cipher_read, .write = cipher_write,
    .unlocked_ioctl = cipher_ioctl, .llseek = default_llseek,
};
static struct miscdevice cipher_misc = {
    .minor = MISC_DYNAMIC_MINOR, .name = "cipherdev", .fops = &cipher_fops,
    .mode = 0600,
};
static int __init cipher_init(void)
{
    int ret = misc_register(&cipher_misc);
    if (ret) { pr_err("cipherdev: register failed: %d\n", ret); return ret; }
    pr_info("cipherdev: module loaded, device /dev/cipherdev\n");
    return 0;
}
static void __exit cipher_exit(void)
{
    misc_deregister(&cipher_misc);
    pr_info("cipherdev: module unloaded\n");
}
module_init(cipher_init);
module_exit(cipher_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ARM64 coursework lab");
MODULE_DESCRIPTION("Stateful character device with ioctl processing modes");
