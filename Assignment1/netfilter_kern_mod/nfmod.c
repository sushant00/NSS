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

static int nfmod_init(void);
static void nfmod_exit(void);
unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *));

static struct nf_hook_ops nfho;

static int nfmod_init(void) {
    printk(KERN_INFO "nfmod: initialising netfilter kernel module...\n");

    nfho.hook = hook_func;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    
    nf_register_hook(&nfho);

    printk(KERN_INFO "nfmod: initialised\n\n");
    return 0;
}

static void nfmod_exit(void) {
    printk(KERN_INFO "nfmod: uninstalling netfilter kernle module...\n");
    
    nf_unregister_hook(&nfho);
    
    printk(KERN_INFO "nfmod: uninstalled nfmod\n");
}

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *)) {
    struct iphdr *ip_header;
    struct tcphdr *tcp_header;

    unsigned int Flags[6];
    char FlagsStr[7];
    // printk(KERN_INFO "nfmod: hook func called\n");

    ip_header = ip_hdr(skb);

    if(ip_header->protocol == IPPROTO_TCP) {
        // printk(KERN_INFO "nfmod: TCP Packet captured\n");
	tcp_header = tcp_hdr(skb);

        //find the flags set in the packet
        FlagsStr[6] = 0;
        if(tcp_header->urg){
            FlagsStr[0] = 'U';
            Flags[0] = 1;
        }else {
            FlagsStr[0] = '-';
            Flags[0] = 0;
        }
        
        if(tcp_header->ack){
            FlagsStr[1] = 'A';
            Flags[1] = 1;
        }else {
            FlagsStr[1] = '-';
            Flags[1] = 0;
        }
        
        if(tcp_header->psh){
            FlagsStr[2] = 'P';
            Flags[2] = 1;
        }else {
            FlagsStr[2] = '-';
            Flags[2] = 0;
        }
        
        if(tcp_header->rst){
            FlagsStr[3] = 'R';
            Flags[3] = 1;
        }else {
            FlagsStr[3] = '-';
            Flags[3] = 0;
        }
        
        if(tcp_header->syn){
            FlagsStr[4] = 'S';
            Flags[4] = 1;
        }else {
            FlagsStr[4] = '-';
            Flags[4] = 0;
        }
        
        if(tcp_header->fin){
            FlagsStr[5] = 'F';
            Flags[5] = 1;
        }else {
            FlagsStr[5] = '-';
            Flags[5] = 0;
        }

      	// printk(KERN_INFO "nfmod: TCP Source Port: %u\n", tcp_header->source);
        // printk(KERN_INFO "nfmod: TCP Destination Port: %u\n", tcp_header->dest);
        // printk(KERN_INFO "nfmod: TCP Packet Flags %s\n", FlagsStr);

        if(!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && !Flags[5] ) {
            printk(KERN_INFO "nfmod: TCP NULL SCAN Source Port: %u Dest Port: %u\n", tcp_header->source, tcp_header->dest);
        } else if (!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && Flags[4] && !Flags[5]) {
            printk(KERN_INFO "nfmod: TCP SYN SCAN Source Port: %u Dest Port: %u\n", tcp_header->source, tcp_header->dest);

        } else if (!Flags[0] && Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && !Flags[5]) {
            printk(KERN_INFO "nfmod: TCP ACK SCAN Source Port: %u Dest Port: %u\n", tcp_header->source, tcp_header->dest);
            
        } else if (!Flags[0] && !Flags[1] && !Flags[2] && !Flags[3] && !Flags[4] && Flags[5]) {
            printk(KERN_INFO "nfmod: TCP FIN SCAN Source Port: %u Dest Port: %u\n", tcp_header->source, tcp_header->dest);
            
        } else if (Flags[0] && !Flags[1] && Flags[2] && !Flags[3] && !Flags[4] && Flags[5] ) {
            printk(KERN_INFO "nfmod: TCP XMAS SCAN Source Port: %u Dest Port: %u\n", tcp_header->source, tcp_header->dest);
            
        }
    }

//    printk(KERN_INFO "nfmod: exiting hook func\n\n");
    return NF_ACCEPT;
}

module_init(nfmod_init);
module_exit(nfmod_exit);

