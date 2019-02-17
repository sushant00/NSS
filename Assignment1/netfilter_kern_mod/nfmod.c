//Sushant Kumar Singh
//2016103

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>

MODULE_AUTHOR("Sushant Kumar Singh");
MODULE_DESCRIPTION("Netfilter module to identify and log type of network reconnaissance packet");

static int nfmod_init();
static void nfmod_exit();
unsigned int hook_func(unsigned int hooknum, struct sk_buff **skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *));

static struct nf_hooks_ops nfho;

static int nfmod_init(void) {
    printk(KERN_INFO, "nfmod: initialising netfilter kernel module...\n");

    nfho.hook = hook_func;
    nfho.hooknu = NF_IP_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    
    nf_register_hook(&nfho);

    printk(KERN_INFO, "nfmod: initialised\n");
    return 0;
}

static void nfmod_exit(void) {
    printk(KERN_INFO, "nfmod: uninstalling netfilter kernle module...\n");
    
    nf_unregister_hook(&nfho);
    
    printk(KERN_INFO, "nfmod: uninstalled nfmod\n");
}

unsigned int hook_func(unsigned int hooknum, struct sk_buff **skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *)) {
    printk("nfmod: hook func called\n");

    struct iphdr *ip_header = ip_hdr(skb);

    if(ip_header->protocol == IPPROTO_TCP) {
        printk(KERN_INFO, "nfmod: TCP Packet captured\n");
        struct tcphdr *tcp_header = tcp_hdr(skb);

        printk(KERN_INFO, "nfmod: TCP Source Port: %u\n", tcp_header->source);
        printk(KERN_INFO, "nfmod: TCP Destination Port: %u\n", tcp_header->dest);

        //find the flags set in the packet
        unsigned int Flags[6];
        char FlagsStr[6];
        if(tcp_header->urg){
            FlagsStr[0] = 'U';
        }else {
            FlagsStr[0] = '-';
        }
        
        if(tcp_header->urg){
            FlagsStr[1] = 'A';
        }else {
            FlagsStr[1] = '-';
        }
        
        if(tcp_header->urg){
            FlagsStr[2] = 'P';
        }else {
            FlagsStr[2] = '-';
        }
        
        if(tcp_header->urg){
            FlagsStr[3] = 'R';
        }else {
            FlagsStr[3] = '-';
        }
        
        if(tcp_header->urg){
            FlagsStr[4] = 'S';
        }else {
            FlagsStr[4] = '-';
        }
        
        if(tcp_header->urg){
            FlagsStr[5] = 'F';
        }else {
            FlagsStr[5] = '-';
        }

        printk(KERN_INFO, "nfmod: TCP Packet Flags %s\n", FlagsStr);

        if(!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && !Flags[5] ) {
            printk(KERN_INFO, "nfmod: TCP NULL SCAN\n");
        } else if (!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && Flags[4] && !Flags[5]) {
            printk(KERN_INFO, "nfmod: TCP SYN SCAN\n");

        } else if (!Flags[0] && Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && !Flags[5]) {
            printk(KERN_INFO, "nfmod: TCP ACK SCAN\n");
            
        } else if (!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && Flags[5]) {
            printk(KERN_INFO, "nfmod: TCP FIN SCAN\n");
            
        } else if (Flags[0] && !Flags[1] && Flags[2] && !Flags[3] && !Flags[4] && Flags[5] ) {
            printk(KERN_INFO, "nfmod: TCP XMAS SCAN\n");
            
        }
    }

    printk(KERN_INFO, "nfmod: exiting hook func\n");
    return NF_ACCEPT;
}

module_init(nfmod_init);
module_exit(nfmod_exit);

