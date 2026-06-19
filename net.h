#ifndef ORANGEX_NET_H
#define ORANGEX_NET_H

#include "types.h"

#define NET_ETH_ARP  0x0806
#define NET_ETH_IP   0x0800
#define NET_IP_TCP   6
#define NET_IP_UDP   17
#define NET_IP_ICMP  1

typedef struct {
    uint32_t ip;
    uint32_t mask;
    uint32_t gateway;
    uint32_t dns;
    uint8_t  mac[6];
    char     iface[16];
    int      up;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
} net_interface_t;

typedef struct {
    uint32_t id;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  proto;
    uint32_t seq;
    uint32_t ack;
    uint16_t window;
    uint8_t  flags;
    uint8_t  buffer[4096];
    uint32_t buf_len;
    uint32_t state;
    uint32_t rtt;
} net_socket_t;

#define NET_SOCK_CLOSED    0
#define NET_SOCK_LISTEN    1
#define NET_SOCK_SYN_SENT  2
#define NET_SOCK_ESTABLISHED 3
#define NET_SOCK_FIN_WAIT  4

void     net_init(void);
void     net_set_ip(const char* ip);
void     net_set_mask(const char* mask);
void     net_set_gateway(const char* gw);
void     net_set_dns(const char* dns);
void     net_get_info(char* buf, uint32_t len);
uint32_t net_resolve(const char* host);
uint32_t net_ping(uint32_t ip, int count);
uint32_t net_ping_host(const char* host, int count);
int      net_tcp_connect(uint32_t ip, uint16_t port);
int      net_tcp_send(int sock, const uint8_t* data, uint32_t len);
int      net_tcp_recv(int sock, uint8_t* buf, uint32_t len);
void     net_tcp_close(int sock);
int      net_udp_send(uint32_t ip, uint16_t port, const uint8_t* data, uint32_t len);
uint16_t net_checksum(void* data, uint32_t len);
void     net_ifconfig(char* buf, uint32_t len);

#endif
