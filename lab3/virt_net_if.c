#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/proc_fs.h>

static dev_t proc_entry;
static struct proc_dir_entry* entry;

static int package_count = 0;

static char* link = "enp0s3";
module_param(link, charp, 0);

static char* ifname = "vni%d";
static unsigned char data[1500];

static struct net_device_stats stats;

static struct net_device *child = NULL;
struct priv {
    struct net_device *parent;
};

#define MAX_COUNT 500

static int dest_ports[MAX_COUNT];
static int src_ports[MAX_COUNT];
static int id_packets[MAX_COUNT];
static int count_filtered = 0;

static char check_frame(struct sk_buff *skb, unsigned char data_shift) {
    package_count++;
	unsigned char *user_data_ptr = NULL;
    struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
    struct udphdr *udp = NULL;
    int data_len = 0;

	if (IPPROTO_UDP == ip->protocol) {
        udp = (struct udphdr*)((unsigned char*)ip + (ip->ihl * 4));
        printk("src: %d dest: %d\n", udp->source, udp->dest);
        if (udp->dest == 80) {
           dest_ports[count_filtered]=udp->dest;
           src_ports[count_filtered]=udp->source;
           id_packets[count_filtered]=package_count;
           count_filtered++;
           if (count_filtered == MAX_COUNT)
                count_filtered = 0;
           return 1;
        }
        data_len = ntohs(udp->len) - sizeof(struct udphdr);
        user_data_ptr = (unsigned char *)(skb->data + sizeof(struct iphdr)  + sizeof(struct udphdr)) + data_shift;
        memcpy(data, user_data_ptr, data_len);
        data[data_len] = '\0';

        printk("Captured UDP datagram, saddr: %d.%d.%d.%d\n",
                ntohl(ip->saddr) >> 24, (ntohl(ip->saddr) >> 16) & 0x00FF,
                (ntohl(ip->saddr) >> 8) & 0x0000FF, (ntohl(ip->saddr)) & 0x000000FF);
        printk("daddr: %d.%d.%d.%d\n",
                ntohl(ip->daddr) >> 24, (ntohl(ip->daddr) >> 16) & 0x00FF,
                (ntohl(ip->daddr) >> 8) & 0x0000FF, (ntohl(ip->daddr)) & 0x000000FF);

    	printk(KERN_INFO "Data length: %d. Data:", data_len);
        printk("%s", data);
        return 0;

    }
    return 0;
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
   // if (child) {
        
        	if (check_frame(*pskb, 0)) {
                stats.rx_packets++;
                stats.rx_bytes += (*pskb)->len;
            }
        (*pskb)->dev = child;
        return RX_HANDLER_ANOTHER;
    //}   
    return RX_HANDLER_PASS; 
} 

static int open(struct net_device *dev) {
    netif_start_queue(dev);
    printk(KERN_INFO "%s: device opened", dev->name);
    return 0; 
} 

static int stop(struct net_device *dev) {
    netif_stop_queue(dev);
    printk(KERN_INFO "%s: device closed", dev->name);
    return 0; 
} 

static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
    struct priv *priv = netdev_priv(dev);

    if (check_frame(skb, 14)) {
        stats.tx_packets++;
        stats.tx_bytes += skb->len;
    }

    if (priv->parent) {
        skb->dev = priv->parent;
        skb->priority = 1;
        dev_queue_xmit(skb);
        return 0;
    }
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
    return &stats;
} 

static struct net_device_ops crypto_net_device_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_get_stats = get_stats,
    .ndo_start_xmit = start_xmit
};

static void setup(struct net_device *dev) {
    int i;
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &crypto_net_device_ops;

    //fill in the MAC address with a phoney
    for (i = 0; i < ETH_ALEN; i++)
        dev->dev_addr[i] = (char)i;
} 

static ssize_t read_proc(struct file *f, char __user *buf, size_t len, loff_t *offset) {
    char stringg[count_filtered * 200];
    int i;
    int stringg_i = 0;
    
    if (*offset > 0 || count_filtered <= 0) {
        return 0;    
    }  

    for (i = 0; i < count_filtered; i++) {
        stringg_i += sprintf(stringg + stringg_i, "id of packet: %d, dest_port: %d, src_port: %d\n", id_packets[i], dest_ports[i], src_ports[i]);   
    }

    stringg_i += sprintf(stringg + stringg_i, "stat: rx_bytes: %d, rx_packets: %d\n", stats.rx_bytes, stats.rx_packets); 
    
    stringg[stringg_i++] = '\0';
    
    if (len < stringg_i) 
		return -EFAULT;
    
	if (copy_to_user(buf, stringg, stringg_i) != 0) {
		return -EFAULT;
	}

    *offset = stringg_i;
	return stringg_i;
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

int __init vni_init(void) {
    entry = proc_create(THIS_MODULE->name, 0660, NULL, &proc_fops);
	printk(KERN_INFO "%s: proc file has been created\n", THIS_MODULE->name);

    int err = 0;
    struct priv *priv;
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
    if (child == NULL) {
        printk(KERN_ERR "%s: allocate error", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        printk(KERN_ERR "%s: no such net: %s", THIS_MODULE->name, link);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        printk(KERN_ERR "%s: illegal net type", THIS_MODULE->name); 
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
    if ((err = dev_alloc_name(child, child->name))) {
        printk(KERN_ERR "%s: allocate name, error %i", THIS_MODULE->name, err);
        free_netdev(child);
        return -EIO;
    }

    register_netdev(child);
    rtnl_lock();
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
    printk(KERN_INFO "Module %s loaded", THIS_MODULE->name);
    printk(KERN_INFO "%s: create link %s", THIS_MODULE->name, child->name);
    printk(KERN_INFO "%s: registered rx handler for %s", THIS_MODULE->name, priv->parent->name);
    return 0; 
}

void __exit vni_exit(void) {
    
    // proc
	proc_remove(entry);
	printk(KERN_INFO "%s: proc file is deleted\n", THIS_MODULE->name);


    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        printk(KERN_INFO "%s: unregister rx handler for %s", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);
    printk(KERN_INFO "Module %s unloaded", THIS_MODULE->name); 
} 

module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Author");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Description");
