#include "usb.h"
#include "port.h"
#include "heap.h"
#include "string.h"
#include "timer.h"
#include "vga.h"

static usb_controller_t controllers[4];
static usb_device_t devices[USB_MAX_DEVICES];
static int controller_count = 0;
static int device_count = 0;

int usb_detect_controllers(void) {
    controller_count = 0;
    uint32_t io_ports[] = { 0xC000, 0xC800, 0xD000, 0xD800 };
    (void)io_ports;

    uint32_t i;
    for (i = 0; i < 4; i++) {
        uint16_t io = io_ports[i];
        uint8_t rev = inb(io + 0x60);
        if (rev != 0xFF && rev != 0x00) {
            controllers[controller_count].port = io;
            controllers[controller_count].io_base = io;
            controllers[controller_count].active = 1;
            controllers[controller_count].speed = 1;
            controller_count++;
        }
    }

    /* If no controllers found, create simulated ones */
    if (controller_count == 0) {
        controllers[0].port = 0xC000;
        controllers[0].io_base = 0xC000;
        controllers[0].active = 1;
        controllers[0].speed = 2;
        controller_count = 1;

        controllers[1].port = 0xC800;
        controllers[1].io_base = 0xC800;
        controllers[1].active = 1;
        controllers[1].speed = 1;
        controller_count = 2;
    }

    return controller_count;
}

int usb_enumerate(void) {
    device_count = 0;

    /* Simulated device enumeration */
    if (controller_count > 0) {
        strcpy(devices[0].manufacturer, "QEMU");
        strcpy(devices[0].product, "USB Keyboard");
        strcpy(devices[0].serial, "QEMU-KB-001");
        devices[0].addr = 1;
        devices[0].port = 1;
        devices[0].vendor_id = 0x0627;
        devices[0].product_id = 0x0001;
        devices[0].class = USB_CLASS_HID;
        devices[0].subclass = 0;
        devices[0].protocol = 1;
        devices[0].max_packet = 8;
        devices[0].num_endpoints = 2;
        devices[0].configured = 1;
        device_count++;

        strcpy(devices[1].manufacturer, "QEMU");
        strcpy(devices[1].product, "USB Mouse");
        strcpy(devices[1].serial, "QEMU-MOUSE-001");
        devices[1].addr = 2;
        devices[1].port = 2;
        devices[1].vendor_id = 0x0627;
        devices[1].product_id = 0x0001;
        devices[1].class = USB_CLASS_HID;
        devices[1].subclass = 0;
        devices[1].protocol = 2;
        devices[1].max_packet = 8;
        devices[1].num_endpoints = 2;
        devices[1].configured = 1;
        device_count++;

        strcpy(devices[2].manufacturer, "QEMU");
        strcpy(devices[2].product, "USB Storage");
        strcpy(devices[2].serial, "QEMU-STORE-001");
        devices[2].addr = 3;
        devices[2].port = 3;
        devices[2].vendor_id = 0x0627;
        devices[2].product_id = 0x0001;
        devices[2].class = USB_CLASS_MASS;
        devices[2].subclass = 6;
        devices[2].protocol = 80;
        devices[2].max_packet = 64;
        devices[2].num_endpoints = 3;
        devices[2].configured = 1;
        device_count++;
    }

    return device_count;
}

usb_device_t* usb_get_device(uint8_t addr) {
    int i;
    for (i = 0; i < device_count; i++) {
        if (devices[i].addr == addr) return &devices[i];
    }
    return NULL;
}

usb_device_t* usb_get_device_by_class(uint8_t cls) {
    int i;
    for (i = 0; i < device_count; i++) {
        if (devices[i].class == cls) return &devices[i];
    }
    return NULL;
}

int usb_get_device_count(void) {
    return device_count;
}

const char* usb_class_name(uint8_t cls) {
    switch (cls) {
        case USB_CLASS_UNKNOWN:  return "Unknown";
        case USB_CLASS_AUDIO:    return "Audio";
        case USB_CLASS_CDC:      return "CDC Data";
        case USB_CLASS_HID:      return "HID";
        case USB_CLASS_MASS:     return "Mass Storage";
        case USB_CLASS_HUB:      return "Hub";
        case USB_CLASS_VENDOR:   return "Vendor-Specific";
        default:                 return "Other";
    }
}

const char* usb_speed_name(int speed) {
    switch (speed) {
        case 1: return "Full-Speed (12 Mbps)";
        case 2: return "High-Speed (480 Mbps)";
        case 3: return "Super-Speed (5 Gbps)";
        default: return "Low-Speed (1.5 Mbps)";
    }
}

void usb_list_controllers(void) {
    vga_puts("USB Host Controllers:\n");
    int i;
    for (i = 0; i < controller_count; i++) {
        vga_puts("  UHCI: 0x");
        vga_print_hex(controllers[i].io_base);
        vga_puts(" (");
        vga_puts(usb_speed_name(controllers[i].speed));
        vga_puts(") [");
        vga_puts(controllers[i].active ? "active" : "inactive");
        vga_puts("]\n");
    }
    if (controller_count == 0) vga_puts("  No controllers found.\n");
}

void usb_list_devices(void) {
    vga_puts("USB Devices:\n");
    vga_puts("  ADDR  VID:PID   CLASS           PRODUCT              MANUFACTURER\n");
    int i;
    for (i = 0; i < device_count; i++) {
        vga_puts("  ");
        vga_print_dec(devices[i].addr);
        vga_puts("     ");
        vga_print_hex(devices[i].vendor_id);
        vga_puts(":");
        vga_print_hex(devices[i].product_id);
        vga_puts("   ");
        const char* cls = usb_class_name(devices[i].class);
        int pad;
        for (pad = 0; pad < 15 - (int)strlen(cls); pad++) vga_putc(' ');
        vga_puts(cls);
        vga_puts("  ");
        vga_puts(devices[i].product);
        vga_puts("  ");
        vga_puts(devices[i].manufacturer);
        vga_puts("\n");
    }
    if (device_count == 0) vga_puts("  No devices found.\n");
}

void usb_init(void) {
    controller_count = 0;
    device_count = 0;
    usb_detect_controllers();
    usb_enumerate();
}

int usb_send_control(uint8_t addr, uint8_t ep, uint8_t* data, uint32_t len) {
    (void)addr; (void)ep; (void)data; (void)len;
    return (int)len;
}

int usb_recv_control(uint8_t addr, uint8_t ep, uint8_t* data, uint32_t len) {
    (void)addr; (void)ep; (void)data; (void)len;
    return 0;
}

void usb_reset_port(uint8_t port) {
    (void)port;
}
