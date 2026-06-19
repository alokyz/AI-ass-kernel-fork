#include "net.h"
#include "string.h"
#include "heap.h"
#include "timer.h"
#include "vga.h"

static net_interface_t net_iface;
static net_socket_t sockets[32];
static uint32_t ip_counter = 0;

static uint32_t parse_ip(const char* ip) {
    uint32_t a = 0, b = 0, c = 0, d = 0;
    int k = 0;
    while (*ip) {
        uint32_t num = 0;
        while (*ip >= '0' && *ip <= '9') { num = num * 10 + (*ip - '0'); ip++; }
        if (k == 0) a = num; else if (k == 1) b = num; else if (k == 2) c = num; else d = num;
        k++; if (*ip == '.') ip++;
    }
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void net_init(void) {
    uint32_t i;
    memset(&net_iface, 0, sizeof(net_iface));
    net_iface.ip = 0xC0A8012A;
    net_iface.mask = 0xFFFFFF00;
    net_iface.gateway = 0xC0A80101;
    net_iface.dns = 0x08080808;
    net_iface.mac[0] = 0x00; net_iface.mac[1] = 0x1A;
    net_iface.mac[2] = 0x2B; net_iface.mac[3] = 0x3C;
    net_iface.mac[4] = 0x4D; net_iface.mac[5] = 0x5E;
    strcpy(net_iface.iface, "eth0");
    net_iface.up = 1;
    for (i = 0; i < 32; i++) sockets[i].state = NET_SOCK_CLOSED;
}

void net_set_ip(const char* ip) { net_iface.ip = parse_ip(ip); }
void net_set_mask(const char* m) { net_iface.mask = parse_ip(m); }
void net_set_gateway(const char* g) { net_iface.gateway = parse_ip(g); }
void net_set_dns(const char* d) { net_iface.dns = parse_ip(d); }

uint32_t net_resolve(const char* host) {
    if (strcmp(host, "localhost") == 0 || strcmp(host, "127.0.0.1") == 0) return 0x7F000001;
    if (strcmp(host, "google.com") == 0 || strcmp(host, "dns.google") == 0) return 0x08080808;
    if (strcmp(host, "github.com") == 0) return 0x1408EC10;
    if (strcmp(host, "orangepkg.dev") == 0) return 0xC0A80101;
    if (strcmp(host, "kernel.org") == 0) return 0xD83CD700;
    if (strcmp(host, "archive.ubuntu.com") == 0) return 0x36E3770A;
    return parse_ip(host);
}

uint32_t net_ping(uint32_t ip, int count) {
    int received = 0;
    int i;
    uint8_t* b = (uint8_t*)&ip;
    (void)b;
    for (i = 0; i < count; i++) {
        timer_sleep(100);
        received++;
        net_iface.rx_packets++;
    }
    return (uint32_t)received;
}

uint32_t net_ping_host(const char* host, int count) {
    return net_ping(net_resolve(host), count);
}

int net_tcp_connect(uint32_t ip, uint16_t port) {
    int i;
    for (i = 0; i < 32; i++) {
        if (sockets[i].state == NET_SOCK_CLOSED) {
            sockets[i].state = NET_SOCK_ESTABLISHED;
            sockets[i].src_port = 1024 + (ip_counter++ % 64000);
            sockets[i].dst_port = port;
            sockets[i].dst_ip = ip;
            sockets[i].seq = 0;
            sockets[i].ack = 0;
            sockets[i].buf_len = 0;
            sockets[i].rtt = 1 + (i % 5);
            net_iface.tx_packets++;
            return i;
        }
    }
    return -1;
}

int net_tcp_send(int sock, const uint8_t* data, uint32_t len) {
    (void)data;
    if (sock < 0 || sock >= 32 || sockets[sock].state == NET_SOCK_CLOSED) return -1;
    sockets[sock].seq += len;
    net_iface.tx_packets++;
    net_iface.tx_bytes += len;
    return (int)len;
}

int net_tcp_recv(int sock, uint8_t* buf, uint32_t len) {
    (void)buf; (void)len;
    if (sock < 0 || sock >= 32 || sockets[sock].state == NET_SOCK_CLOSED) return -1;
    net_iface.rx_packets++;
    return 0;
}

void net_tcp_close(int sock) {
    if (sock >= 0 && sock < 32) { sockets[sock].state = NET_SOCK_CLOSED; net_iface.tx_packets++; }
}

int net_udp_send(uint32_t ip, uint16_t port, const uint8_t* data, uint32_t len) {
    (void)ip; (void)port; (void)data; (void)len;
    net_iface.tx_packets++; net_iface.tx_bytes += len;
    return (int)len;
}

uint16_t net_checksum(void* data, uint32_t len) {
    uint16_t* ptr = (uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *ptr++; len -= 2; }
    if (len) sum += *(uint8_t*)ptr;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void net_get_info(char* buf, uint32_t len) {
    (void)len;
    uint8_t* ip = (uint8_t*)&net_iface.ip;
    uint8_t* gw = (uint8_t*)&net_iface.gateway;
    uint8_t* dns = (uint8_t*)&net_iface.dns;
    char* p = buf;
    const char* s;

    s = "Interface: "; while (*s) *p++ = *s++;
    s = net_iface.iface; while (*s) *p++ = *s++; *p++ = '\n';
    s = "IP: "; while (*s) *p++ = *s++;
    p[0] = '0' + ip[3]; p[1] = '.'; p[2] = '0' + ip[2]; p[3] = '.';
    p[4] = '0' + ip[1]; p[5] = '.'; p[6] = '0' + ip[0]; p[7] = '\n'; p += 8;
    s = "Gateway: "; while (*s) *p++ = *s++;
    p[0] = '0' + gw[3]; p[1] = '.'; p[2] = '0' + gw[2]; p[3] = '.';
    p[4] = '0' + gw[1]; p[5] = '.'; p[6] = '0' + gw[0]; p[7] = '\n'; p += 8;
    s = "DNS: "; while (*s) *p++ = *s++;
    p[0] = '0' + dns[3]; p[1] = '.'; p[2] = '0' + dns[2]; p[3] = '.';
    p[4] = '0' + dns[1]; p[5] = '.'; p[6] = '0' + dns[0]; p[7] = '\n'; p += 8;
    s = "Status: "; while (*s) *p++ = *s++;
    s = net_iface.up ? "UP\n" : "DOWN\n"; while (*s) *p++ = *s++;
    *p = '\0';
}

void net_ifconfig(char* buf, uint32_t len) {
    (void)len;
    uint8_t* ip = (uint8_t*)&net_iface.ip;
    uint8_t* mask = (uint8_t*)&net_iface.mask;
    uint8_t* mac = net_iface.mac;
    char* p = buf;
    const char* s;
    int i;

    s = net_iface.iface; while (*s) *p++ = *s++;
    s = ": flags=4163<UP,BROADCAST,RUNNING,MULTICAST>\n"; while (*s) *p++ = *s++;
    s = "    inet "; while (*s) *p++ = *s++;
    p[0] = '0' + ip[3]; p[1] = '.'; p[2] = '0' + ip[2]; p[3] = '.';
    p[4] = '0' + ip[1]; p[5] = '.'; p[6] = '0' + ip[0]; p[7] = ' '; p += 8;
    s = "netmask "; while (*s) *p++ = *s++;
    p[0] = '0' + mask[3]; p[1] = '.'; p[2] = '0' + mask[2]; p[3] = '.';
    p[4] = '0' + mask[1]; p[5] = '.'; p[6] = '0' + mask[0]; p[7] = '\n'; p += 8;
    s = "    ether "; while (*s) *p++ = *s++;
    char hex[] = "0123456789abcdef";
    for (i = 0; i < 6; i++) { p[0] = hex[(mac[i]>>4)&0xF]; p[1] = hex[mac[i]&0xF]; p += 2; if (i < 5) *p++ = ':'; }
    *p++ = '\n';
    s = "    RX packets: "; while (*s) *p++ = *s++;
    vga_print_dec(net_iface.rx_packets);
    s = "\n    TX packets: "; while (*s) *p++ = *s++;
    *p = '\0';
}
