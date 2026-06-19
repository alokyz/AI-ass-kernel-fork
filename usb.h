#ifndef ORANGEX_USB_H
#define ORANGEX_USB_H

#include "types.h"

#define USB_PORT_DATA    0x60
#define USB_PORT_STATUS  0x64
#define USB_CMD_RESET    0x02
#define USB_CMD_RUN      0x01
#define USB_CMD_GSORE    0x08
#define USB_CMD_EHRESET  0x10
#define USB_CMD_HCRESET  0x04

#define USB_MAX_DEVICES  16
#define USB_MAX_ENDPOINTS 4

typedef enum {
    USB_CLASS_UNKNOWN  = 0x00,
    USB_CLASS_AUDIO    = 0x01,
    USB_CLASS_CDC      = 0x02,
    USB_CLASS_HID      = 0x03,
    USB_CLASS_MASS     = 0x08,
    USB_CLASS_HUB      = 0x09,
    USB_CLASS_VENDOR   = 0xFF,
} usb_class_t;

typedef struct {
    uint8_t  addr;
    uint8_t  port;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t  class;
    uint8_t  subclass;
    uint8_t  protocol;
    uint8_t  max_packet;
    char     manufacturer[64];
    char     product[64];
    char     serial[64];
    uint8_t  num_endpoints;
    int      configured;
} usb_device_t;

typedef struct {
    uint8_t  address;
    uint8_t  endpoint;
    uint8_t  type;
    uint8_t  interval;
    uint16_t max_packet;
    uint8_t  direction;
} usb_endpoint_t;

typedef struct {
    uint32_t port;
    uint32_t io_base;
    int      active;
    int      speed;
} usb_controller_t;

void     usb_init(void);
int      usb_detect_controllers(void);
int      usb_enumerate(void);
usb_device_t* usb_get_device(uint8_t addr);
usb_device_t* usb_get_device_by_class(uint8_t cls);
int      usb_get_device_count(void);
void     usb_list_devices(void);
void     usb_list_controllers(void);
int      usb_send_control(uint8_t addr, uint8_t ep, uint8_t* data, uint32_t len);
int      usb_recv_control(uint8_t addr, uint8_t ep, uint8_t* data, uint32_t len);
void     usb_reset_port(uint8_t port);
const char* usb_class_name(uint8_t cls);
const char* usb_speed_name(int speed);

#endif
