#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
//#include <linux/string.h>
#include <linux/uaccess.h>
//#include <linux/version.h>
//#include <linux/types.h>
//#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("prilutskaya");
MODULE_DESCRIPTION("IO lab1. Charcter device driver");
MODULE_VERSION("0.1");

#define PROC_BUF_MAX_SIZE 1024

static dev_t proc_entry;
static struct proc_dir_entry* entry;
static struct cdev c_dev; 
static struct class *cl;
static unsigned long proc_buf[PROC_BUF_MAX_SIZE];
static unsigned int proc_buf_size = 0;

static int drv_open(struct inode *i, struct file *f) {
  printk(KERN_INFO "%s: open()\n", THIS_MODULE->name);
  return 0;
}

static int drv_close(struct inode *i, struct file *f) {
  printk(KERN_INFO "%s: close()\n", THIS_MODULE->name);
  return 0;
}

static ssize_t write_sums(struct file *f, char __user *buf, size_t len, loff_t *offset, int read_to_user){
    char sums[proc_buf_size * sizeof(int)];
    int i;
    int sums_i = 0;
    
    if (*offset > 0 || proc_buf_size <= 0) {
        return 0;    
    }  

    for (i = 0; i < proc_buf_size; i++) {
        sums_i += sprintf(sums + sums_i, "%ld\n", proc_buf[i]);   
    }
    
    sums[sums_i++] = '\0';
    
    if (len < sums_i) 
		return -EFAULT;
    
    if (read_to_user) {
	    if (copy_to_user(buf, sums, sums_i) != 0) {
		    return -EFAULT;
	    }
    } else {
        if (printk(KERN_INFO "%s", sums) == 0) {
		    return -EFAULT;
	    }
    }

    *offset = sums_i;
	return sums_i;
}

static ssize_t read_dev(struct file *f, char __user *buf, size_t len, loff_t *offset) {
    return write_sums(f, buf, len, offset, 0);
}

static ssize_t read_proc(struct file *f, char __user *buf, size_t len, loff_t *offset) {
    return write_sums(f, buf, len, offset, 1);
}

static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos) {
	printk(KERN_DEBUG "%s: Attempt to write proc file", THIS_MODULE->name);
	return -1;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read = read_proc,
	.write = proc_write,
};

static ssize_t char_write(struct file *f, const char __user *buf,  size_t len, loff_t *offset) {

    char c;
    unsigned long sum  = 0;
    unsigned long current_num = 0;
    int i;

    if (copy_from_user(&c, buf, 1) != 0) {
            return -EFAULT;
    }

    if (c == 'r') {
        char filename[256];
        copy_from_user(&filename, buf + 2, len - 2);
        filename[len - 3]='\0';
        proc_remove(entry);
	    printk(KERN_INFO "%s: proc file is deleted\n", THIS_MODULE->name);
        entry = proc_create(filename, 0660, NULL, &proc_fops);
	    printk(KERN_INFO "%s: proc file has been created\n", filename);
        return len;
    }

    for (i = 0; i < len; i++){
        if (copy_from_user(&c, buf + i, 1) != 0) {
            return -EFAULT;
        }
        if (c >= '0' && c <= '9') { 
            current_num = current_num * 10 + c - '0';
        }
        else {
            sum += current_num;
            current_num = 0;        
        }  
    }
    sum += current_num;

    proc_buf[proc_buf_size++] = sum;
    if (proc_buf_size > PROC_BUF_MAX_SIZE) {
        proc_buf_size = 0;
        memset(proc_buf, 0, PROC_BUF_MAX_SIZE * sizeof(proc_buf[0]));
    }
    *offset = len;
    return len;
}

static struct file_operations char_fops = {
  .owner = THIS_MODULE,
  .open = drv_open,
  .release = drv_close,
  .read = read_dev,
  .write = char_write
};

static int __init drv_init(void) {
    entry = proc_create(THIS_MODULE->name, 0660, NULL, &proc_fops);
	printk(KERN_INFO "%s: proc file has been created\n", THIS_MODULE->name);
	
    if (alloc_chrdev_region(&proc_entry, 0, 1, "var3") < 0) {
		return -1;
	}

    if ((cl = class_create(THIS_MODULE, "var3")) == NULL) {
		unregister_chrdev_region(proc_entry, 1);
		return -1;
	}

    if (device_create(cl, NULL, proc_entry, NULL, "var3") == NULL) {
		class_destroy(cl);
		unregister_chrdev_region(proc_entry, 1);
		return -1;
	}

    cdev_init(&c_dev, &char_fops);

    if (cdev_add(&c_dev, proc_entry, 1) == -1) {
		device_destroy(cl, proc_entry);
		class_destroy(cl);
		unregister_chrdev_region(proc_entry, 1);
		return -1;
	}

    return 0;
}

static void __exit drv_exit(void) {
    // proc
	proc_remove(entry);
	printk(KERN_INFO "%s: proc file is deleted\n", THIS_MODULE->name);

    // char
    cdev_del(&c_dev);
    device_destroy(cl, proc_entry);
    class_destroy(cl);
    unregister_chrdev_region(proc_entry, 1);
    printk(KERN_INFO "%s: bye!\n", THIS_MODULE->name);
}

module_init(drv_init);
module_exit(drv_exit);

