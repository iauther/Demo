//Chirp Microsystems Firmware Header Generator
//File generated from robo_floor_v13.hex at 2020-05-06 14:27:09.318000 by klong

#include <stdint.h>
#include "dev/ch101/inc/ch101_def.h"
#include "dev/ch101/inc/ch101_floor.h"

const char * ch101_floor_version = "robo_floor_v13.hex";

#define RAM_INIT_ADDRESS 1160

#define RAM_INIT_WRITE_SIZE   20

uint16_t get_ch101_floor_fw_ram_init_addr(void) { return (uint16_t)RAM_INIT_ADDRESS;}
uint16_t get_ch101_floor_fw_ram_init_size(void) { return (uint16_t)RAM_INIT_WRITE_SIZE;}

const unsigned char ram_ch101_floor_init[RAM_INIT_WRITE_SIZE] = {
0x00, 0xFA, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x00, 0x00, 
0x64, 0x00, 0x00, 0x01, };

const unsigned char * get_ram_ch101_floor_init_ptr(void) { return &ram_ch101_floor_init[0];}

const unsigned char ch101_floor_fw[CH101_FW_SIZE] = {
0x0f, 0x12, 0x0e, 0x12, 0x0d, 0x12, 0x0c, 0x12, 0x0b, 0x12, 0x0a, 0x12, 0x09, 0x12, 0x08, 0x12, 
0x07, 0x12, 0x06, 0x12, 0xd2, 0xc3, 0x96, 0x04, 0xc2, 0x93, 0x14, 0x02, 0x0a, 0x20, 0xb0, 0x12, 
0x10, 0xfb, 0x4c, 0x93, 0x27, 0x20, 0xd2, 0x43, 0x14, 0x02, 0xb2, 0x40, 0x04, 0x38, 0x08, 0x02, 
0x21, 0x3c, 0xd2, 0x93, 0x14, 0x02, 0x1c, 0x20, 0x1c, 0x42, 0x36, 0x02, 0x1d, 0x42, 0x34, 0x02, 
0xb0, 0x12, 0x66, 0xfc, 0x1c, 0x92, 0x88, 0x04, 0x0e, 0x28, 0x92, 0x83, 0x90, 0x04, 0xc2, 0x43, 
0x86, 0x04, 0xd2, 0x43, 0x01, 0x02, 0xe2, 0x43, 0x14, 0x02, 0xe2, 0xd3, 0x96, 0x04, 0xb2, 0x40, 
0x80, 0x10, 0xd0, 0x01, 0x07, 0x3c, 0x82, 0x4c, 0x88, 0x04, 0x92, 0x53, 0x90, 0x04, 0x02, 0x3c, 
0x82, 0x43, 0xf0, 0x01, 0xf2, 0x90, 0x03, 0x00, 0x86, 0x04, 0x34, 0x2c, 0xe2, 0x93, 0x14, 0x02, 
0x31, 0x20, 0x5f, 0x42, 0x07, 0x02, 0x5e, 0x42, 0x86, 0x04, 0xb2, 0x90, 0x32, 0x00, 0x8a, 0x04, 
0x02, 0x2c, 0x92, 0x53, 0x8a, 0x04, 0x5f, 0x92, 0x04, 0x02, 0x02, 0x2c, 0xc2, 0x4f, 0x04, 0x02, 
0x58, 0x42, 0x04, 0x02, 0x5e, 0x42, 0x11, 0x02, 0x06, 0x43, 0x07, 0x43, 0x0e, 0x98, 0x16, 0x2c, 
0x3f, 0x40, 0x1c, 0x02, 0x0a, 0x4e, 0x0a, 0x5a, 0x1a, 0x53, 0x0a, 0x5a, 0x0a, 0x5f, 0x09, 0x4e, 
0x09, 0x59, 0x09, 0x59, 0x09, 0x5f, 0x08, 0x8e, 0x2c, 0x49, 0x2d, 0x4a, 0xb0, 0x12, 0x66, 0xfc, 
0x06, 0x5c, 0x07, 0x63, 0x2a, 0x52, 0x29, 0x52, 0x18, 0x83, 0xf6, 0x23, 0x82, 0x46, 0x18, 0x02, 
0x82, 0x47, 0x1a, 0x02, 0xe2, 0x93, 0x14, 0x02, 0x11, 0x28, 0xd2, 0xd3, 0xe0, 0x01, 0xd2, 0xc3, 
0xe0, 0x01, 0xd2, 0xb3, 0x96, 0x04, 0x0f, 0x20, 0xb2, 0x40, 0x77, 0x06, 0xa6, 0x01, 0x3c, 0x42, 
0xb0, 0x12, 0xae, 0xfd, 0xb2, 0x40, 0x77, 0x01, 0xa6, 0x01, 0x05, 0x3c, 0x5c, 0x43, 0xb0, 0x12, 
0x26, 0xfa, 0xa2, 0xc2, 0x92, 0x01, 0xa2, 0xd2, 0x92, 0x01, 0xd2, 0x42, 0x84, 0x04, 0xe0, 0x01, 
0xb1, 0xc0, 0xf0, 0x00, 0x14, 0x00, 0x36, 0x41, 0x37, 0x41, 0x38, 0x41, 0x39, 0x41, 0x3a, 0x41, 
0x3b, 0x41, 0x3c, 0x41, 0x3d, 0x41, 0x3e, 0x41, 0x3f, 0x41, 0x00, 0x13, 0x0a, 0x12, 0xb2, 0x40, 
0x80, 0x5a, 0x20, 0x01, 0xe2, 0x42, 0xe0, 0x01, 0xd2, 0x43, 0xe2, 0x01, 0xf2, 0x40, 0x40, 0x00, 
0x01, 0x02, 0xf2, 0x40, 0x78, 0x00, 0x07, 0x02, 0xc2, 0x43, 0x15, 0x02, 0xf2, 0x40, 0x2c, 0x00, 
0x10, 0x02, 0xf2, 0x40, 0x2c, 0x00, 0x12, 0x02, 0xf2, 0x40, 0x1e, 0x00, 0x04, 0x02, 0xc2, 0x43, 
0x00, 0x02, 0xf2, 0x42, 0x15, 0x02, 0xd2, 0x43, 0x05, 0x02, 0xf2, 0x40, 0x14, 0x00, 0x11, 0x02, 
0xb2, 0x40, 0x00, 0x01, 0x02, 0x02, 0xf2, 0x40, 0x03, 0x00, 0xc2, 0x01, 0xb2, 0x40, 0x00, 0x02, 
0xa6, 0x01, 0x3c, 0x42, 0xb0, 0x12, 0xae, 0xfd, 0xb2, 0x40, 0x00, 0x06, 0xa6, 0x01, 0x3c, 0x40, 
0x3c, 0x00, 0xb0, 0x12, 0xae, 0xfd, 0xb2, 0x40, 0x1c, 0x02, 0xb0, 0x01, 0x3f, 0x40, 0x07, 0x00, 
0x82, 0x4f, 0xb2, 0x01, 0xb2, 0x40, 0x77, 0x01, 0xa6, 0x01, 0xb2, 0x40, 0x00, 0x01, 0x90, 0x01, 
0x82, 0x4f, 0x92, 0x01, 0x0a, 0x43, 0x05, 0x3c, 0xc2, 0x93, 0x96, 0x04, 0x02, 0x24, 0x32, 0xd0, 
0x18, 0x00, 0x5f, 0x42, 0x01, 0x02, 0x0a, 0x9f, 0x20, 0x24, 0x5a, 0x42, 0x01, 0x02, 0x0f, 0x4a, 
0x3f, 0x80, 0x10, 0x00, 0x18, 0x24, 0x3f, 0x80, 0x10, 0x00, 0x15, 0x24, 0x3f, 0x80, 0x20, 0x00, 
0x0d, 0x20, 0xc2, 0x43, 0x14, 0x02, 0xe2, 0x42, 0x86, 0x04, 0xb2, 0x40, 0x1e, 0x18, 0x08, 0x02, 
0x92, 0x42, 0x98, 0x04, 0xf0, 0x01, 0x5c, 0x43, 0xb0, 0x12, 0x26, 0xfa, 0xe2, 0x42, 0x84, 0x04, 
0xe2, 0xc3, 0xe0, 0x01, 0x02, 0x3c, 0xe2, 0xd3, 0xe0, 0x01, 0xc2, 0x93, 0x96, 0x04, 0xd4, 0x23, 
0x32, 0xd0, 0x58, 0x00, 0xd6, 0x3f, 0x0a, 0x12, 0x0a, 0x4c, 0xf2, 0x90, 0x40, 0x00, 0x01, 0x02, 
0x0c, 0x24, 0xd2, 0xb3, 0x96, 0x04, 0x09, 0x20, 0xb2, 0x40, 0x77, 0x06, 0xa6, 0x01, 0x3c, 0x42, 
0xb0, 0x12, 0xae, 0xfd, 0xb2, 0x40, 0x77, 0x01, 0xa6, 0x01, 0xd2, 0xd3, 0x96, 0x04, 0x5f, 0x42, 
0x15, 0x02, 0x8f, 0x11, 0x1f, 0x52, 0x98, 0x04, 0x82, 0x4f, 0xf0, 0x01, 0xf2, 0x90, 0x40, 0x00, 
0x01, 0x02, 0x24, 0x24, 0xd2, 0x92, 0x07, 0x02, 0x92, 0x04, 0x25, 0x24, 0xd2, 0x42, 0x07, 0x02, 
0x92, 0x04, 0x5e, 0x42, 0x07, 0x02, 0x3e, 0x80, 0x0a, 0x00, 0xb2, 0x40, 0x02, 0x44, 0x74, 0x04, 
0x5f, 0x42, 0x10, 0x02, 0x8f, 0x10, 0x3f, 0xf0, 0x00, 0xfc, 0x3f, 0x50, 0x12, 0x00, 0x82, 0x4f, 
0x76, 0x04, 0x5f, 0x42, 0x12, 0x02, 0x8f, 0x10, 0x3f, 0xf0, 0x00, 0xfc, 0x0e, 0x5e, 0x0f, 0x5e, 
0x82, 0x4f, 0x78, 0x04, 0xf2, 0x40, 0x03, 0x00, 0x87, 0x04, 0x05, 0x3c, 0xb2, 0x40, 0x40, 0x20, 
0x74, 0x04, 0xd2, 0x43, 0x87, 0x04, 0x4a, 0x93, 0x04, 0x20, 0xb2, 0x40, 0x82, 0x10, 0xa2, 0x01, 
0x03, 0x3c, 0xb2, 0x40, 0x86, 0x10, 0xa2, 0x01, 0x5f, 0x42, 0x87, 0x04, 0x0f, 0x93, 0x06, 0x24, 
0x3e, 0x40, 0x74, 0x04, 0xb2, 0x4e, 0xa4, 0x01, 0x1f, 0x83, 0xfc, 0x23, 0x92, 0x42, 0x08, 0x02, 
0xa0, 0x01, 0xc2, 0x93, 0x14, 0x02, 0x03, 0x24, 0xb2, 0xd0, 0x00, 0x08, 0xa2, 0x01, 0x92, 0x43, 
0xae, 0x01, 0xa2, 0x43, 0xae, 0x01, 0xc2, 0x93, 0x14, 0x02, 0x08, 0x24, 0x3f, 0x40, 0x00, 0x30, 
0x1f, 0x52, 0x90, 0x04, 0x1e, 0x42, 0xa8, 0x01, 0x82, 0x4f, 0xa0, 0x01, 0x3a, 0x41, 0x30, 0x41, 
0x1d, 0x42, 0x36, 0x02, 0x1e, 0x42, 0x34, 0x02, 0x1d, 0x93, 0x04, 0x34, 0x0f, 0x4d, 0x3f, 0xe3, 
0x1f, 0x53, 0x01, 0x3c, 0x0f, 0x4d, 0x1e, 0x93, 0x02, 0x34, 0x3e, 0xe3, 0x1e, 0x53, 0x0e, 0x9f, 
0x03, 0x2c, 0x0c, 0x4e, 0x0e, 0x4f, 0x0f, 0x4c, 0x12, 0xc3, 0x0f, 0x10, 0x0f, 0x11, 0x0f, 0x5e, 
0x1c, 0x43, 0x1f, 0x92, 0x8c, 0x04, 0x18, 0x28, 0x0d, 0x11, 0x0d, 0x11, 0x1d, 0x82, 0x34, 0x02, 
0x1d, 0x93, 0x02, 0x38, 0x3f, 0x43, 0x01, 0x3c, 0x1f, 0x43, 0xc2, 0x93, 0x8e, 0x04, 0x07, 0x24, 
0x5e, 0x42, 0x8e, 0x04, 0x8e, 0x11, 0x0f, 0x9e, 0x02, 0x24, 0x0c, 0x43, 0x02, 0x3c, 0x82, 0x5f, 
0x98, 0x04, 0xc2, 0x4f, 0x8e, 0x04, 0x30, 0x41, 0xb2, 0x50, 0x14, 0x00, 0x98, 0x04, 0xb2, 0x90, 
0x2d, 0x01, 0x98, 0x04, 0x06, 0x28, 0xb2, 0x80, 0xc8, 0x00, 0x98, 0x04, 0x12, 0xc3, 0x12, 0x10, 
0x8c, 0x04, 0xc2, 0x43, 0x8e, 0x04, 0x30, 0x41, 0x0f, 0x12, 0x5f, 0x42, 0x8f, 0x04, 0x0f, 0x93, 
0x15, 0x24, 0x1f, 0x83, 0x26, 0x24, 0x1f, 0x83, 0x29, 0x20, 0xb2, 0x90, 0x16, 0x00, 0x82, 0x04, 
0x07, 0x2c, 0x1f, 0x42, 0x82, 0x04, 0xdf, 0x42, 0xc1, 0x01, 0x00, 0x02, 0x92, 0x53, 0x82, 0x04, 
0xd2, 0x83, 0x85, 0x04, 0x1b, 0x20, 0xc2, 0x43, 0x8f, 0x04, 0x18, 0x3c, 0x5f, 0x42, 0xc1, 0x01, 
0x82, 0x4f, 0x82, 0x04, 0xd2, 0x43, 0x8f, 0x04, 0xd2, 0x4f, 0x00, 0x02, 0xc0, 0x01, 0x3f, 0x90, 
0x06, 0x00, 0x0c, 0x20, 0xf2, 0x40, 0x24, 0x00, 0xe0, 0x01, 0xb2, 0x40, 0x03, 0x00, 0xd8, 0x01, 
0x05, 0x3c, 0xd2, 0x42, 0xc1, 0x01, 0x85, 0x04, 0xe2, 0x43, 0x8f, 0x04, 0xf2, 0xd0, 0x10, 0x00, 
0xc2, 0x01, 0xf2, 0xd0, 0x20, 0x00, 0xc2, 0x01, 0xb1, 0xc0, 0xf0, 0x00, 0x02, 0x00, 0x3f, 0x41, 
0x00, 0x13, 0x0f, 0x12, 0x0e, 0x12, 0x0d, 0x12, 0x0c, 0x12, 0x0b, 0x12, 0x92, 0x42, 0x02, 0x02, 
0x90, 0x01, 0xe2, 0x93, 0x01, 0x02, 0x03, 0x20, 0xd2, 0x83, 0x9b, 0x04, 0x0d, 0x24, 0xd2, 0xb3, 
0x96, 0x04, 0x10, 0x20, 0xb2, 0x40, 0x77, 0x06, 0xa6, 0x01, 0x3c, 0x42, 0xb0, 0x12, 0xae, 0xfd, 
0xb2, 0x40, 0x77, 0x01, 0xa6, 0x01, 0x06, 0x3c, 0xd2, 0x42, 0x05, 0x02, 0x9b, 0x04, 0x5c, 0x43, 
0xb0, 0x12, 0x26, 0xfa, 0xb1, 0xc0, 0xf0, 0x00, 0x0a, 0x00, 0x3b, 0x41, 0x3c, 0x41, 0x3d, 0x41, 
0x3e, 0x41, 0x3f, 0x41, 0x00, 0x13, 0x0a, 0x12, 0x1d, 0x93, 0x03, 0x34, 0x3d, 0xe3, 0x1d, 0x53, 
0x02, 0x3c, 0x3c, 0xe3, 0x1c, 0x53, 0x0e, 0x4d, 0x0f, 0x4c, 0x0e, 0x11, 0x0f, 0x11, 0x0b, 0x43, 
0x0c, 0x4e, 0x0d, 0x4b, 0xb0, 0x12, 0x1e, 0xfd, 0x0a, 0x4c, 0x0c, 0x4f, 0x0d, 0x4b, 0xb0, 0x12, 
0x1e, 0xfd, 0x1f, 0x93, 0x03, 0x34, 0x0e, 0x8c, 0x0f, 0x5a, 0x02, 0x3c, 0x0e, 0x5c, 0x0f, 0x8a, 
0x1b, 0x53, 0x2b, 0x92, 0xed, 0x3b, 0x0c, 0x4e, 0x3a, 0x41, 0x30, 0x41, 0x0f, 0x12, 0x0e, 0x12, 
0x0d, 0x12, 0x0c, 0x12, 0x0b, 0x12, 0xe2, 0xb3, 0xe0, 0x01, 0x12, 0x24, 0xd2, 0x42, 0xe0, 0x01, 
0x84, 0x04, 0xe2, 0xc3, 0xe0, 0x01, 0xa2, 0xc2, 0x92, 0x01, 0x4c, 0x43, 0xf2, 0x90, 0x20, 0x00, 
0x01, 0x02, 0x01, 0x24, 0x5c, 0x43, 0xb0, 0x12, 0x26, 0xfa, 0xb1, 0xc0, 0xf0, 0x00, 0x0a, 0x00, 
0x3b, 0x41, 0x3c, 0x41, 0x3d, 0x41, 0x3e, 0x41, 0x3f, 0x41, 0x00, 0x13, 0x0f, 0x12, 0xc2, 0x43, 
0x8f, 0x04, 0x92, 0x53, 0x82, 0x04, 0xb2, 0x90, 0x74, 0x02, 0x82, 0x04, 0x03, 0x28, 0x82, 0x43, 
0x82, 0x04, 0x05, 0x3c, 0x1f, 0x42, 0x82, 0x04, 0xd2, 0x4f, 0x00, 0x02, 0xc0, 0x01, 0xf2, 0xd0, 
0x20, 0x00, 0xc2, 0x01, 0xb1, 0xc0, 0xf0, 0x00, 0x02, 0x00, 0x3f, 0x41, 0x00, 0x13, 0x3d, 0xf0, 
0x0f, 0x00, 0x3d, 0xe0, 0x0f, 0x00, 0x0d, 0x5d, 0x00, 0x5d, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 
0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 
0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x0c, 0x11, 0x30, 0x41, 0x0f, 0x12, 0xb2, 0xf0, 0xef, 0xff, 
0xa2, 0x01, 0x3f, 0x40, 0x00, 0x30, 0x1f, 0x52, 0x90, 0x04, 0x82, 0x4f, 0xa0, 0x01, 0xb1, 0xc0, 
0xf0, 0x00, 0x02, 0x00, 0x3f, 0x41, 0x00, 0x13, 0x92, 0x42, 0xda, 0x01, 0x0a, 0x02, 0x82, 0x43, 
0xd8, 0x01, 0xe2, 0x42, 0xe0, 0x01, 0xb1, 0xc0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x13, 0x31, 0x40, 
0x00, 0x0a, 0xb0, 0x12, 0xbc, 0xfd, 0x0c, 0x43, 0xb0, 0x12, 0x3c, 0xf9, 0xb0, 0x12, 0xc0, 0xfd, 
0xe2, 0xc3, 0x96, 0x04, 0x92, 0x42, 0xd2, 0x01, 0x16, 0x02, 0xb1, 0xc0, 0xf0, 0x00, 0x00, 0x00, 
0x00, 0x13, 0xd2, 0xc3, 0x96, 0x04, 0xb1, 0xc0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x13, 0x1c, 0x83, 
0x03, 0x43, 0xfd, 0x23, 0x30, 0x41, 0x32, 0xd0, 0x10, 0x00, 0xfd, 0x3f, 0x1c, 0x43, 0x30, 0x41, 
0x03, 0x43, 0xff, 0x3f, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xec, 0xfc, 0x98, 0xfb, 0xc4, 0xfd, 0x68, 0xfd, 0xac, 0xfc, 0x00, 0x00, 0xb6, 0xfd, 0x00, 0xf8, 
0x4a, 0xfd, 0xa2, 0xfd, 0xb6, 0xfd, 0x00, 0x00, 0x90, 0xfd, 0x12, 0xfc, 0xb6, 0xfd, 0x7e, 0xfd, 
};
