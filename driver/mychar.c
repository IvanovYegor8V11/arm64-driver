#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "mychar"
#define BUFFER_SIZE 256

static int major_number;

static char device_buffer[BUFFER_SIZE] =
    "Hello from ARM64 character driver!\n";

static size_t buffer_size =
    sizeof("Hello from ARM64 character driver!\n") - 1;

static DEFINE_MUTEX(device_mutex);

/*
 * Открытие устройства
 */
static int mychar_open(struct inode *inode, struct file *file)
{
    pr_info("mychar: device opened\n");

    return 0;
}

/*
 * Закрытие устройства
 */
static int mychar_release(struct inode *inode, struct file *file)
{
    pr_info("mychar: device closed\n");

    return 0;
}

/*
 * Чтение данных из устройства
 */
static ssize_t mychar_read(
    struct file *file,
    char __user *user_buffer,
    size_t count,
    loff_t *offset)
{
    ssize_t bytes_read;

    pr_info("mychar: read requested, count=%zu\n", count);

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    bytes_read = simple_read_from_buffer(
        user_buffer,
        count,
        offset,
        device_buffer,
        buffer_size
    );

    mutex_unlock(&device_mutex);

    pr_info("mychar: read completed, bytes=%zd\n", bytes_read);

    return bytes_read;
}

/*
 * Запись данных в устройство
 */
static ssize_t mychar_write(
    struct file *file,
    const char __user *user_buffer,
    size_t count,
    loff_t *offset)
{
    size_t bytes_to_write;

    pr_info("mychar: write requested, count=%zu\n", count);

    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - 1));

    if (mutex_lock_interruptible(&device_mutex))
        return -ERESTARTSYS;

    if (copy_from_user(
            device_buffer,
            user_buffer,
            bytes_to_write)) {

        mutex_unlock(&device_mutex);

        pr_err("mychar: copy_from_user failed\n");

        return -EFAULT;
    }

    device_buffer[bytes_to_write] = '\0';
    buffer_size = bytes_to_write;

    mutex_unlock(&device_mutex);

    pr_info("mychar: write completed, bytes=%zu\n",
            bytes_to_write);

    return bytes_to_write;
}

/*
 * Таблица файловых операций
 */
static const struct file_operations mychar_fops = {
    .owner   = THIS_MODULE,
    .open    = mychar_open,
    .release = mychar_release,
    .read    = mychar_read,
    .write   = mychar_write,
    .llseek  = default_llseek
};

/*
 * Инициализация драйвера
 */
static int __init mychar_init(void)
{
    pr_info("mychar: initializing driver\n");

    major_number = register_chrdev(
        0,
        DEVICE_NAME,
        &mychar_fops
    );

    if (major_number < 0) {
        pr_err("mychar: failed to register device\n");
        return major_number;
    }

    pr_info("mychar: driver loaded successfully\n");

    pr_info("mychar: registered major number = %d\n",
            major_number);

    return 0;
}

/*
 * Выгрузка драйвера
 */
static void __exit mychar_exit(void)
{
    pr_info("mychar: unloading driver\n");

    unregister_chrdev(
        major_number,
        DEVICE_NAME
    );

    pr_info("mychar: driver unloaded successfully\n");
}

module_init(mychar_init);
module_exit(mychar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ARM64 Kernel Lab");
MODULE_DESCRIPTION("Simple ARM64 character device driver");
MODULE_VERSION("1.0");