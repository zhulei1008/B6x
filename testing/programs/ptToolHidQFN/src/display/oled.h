#ifndef _OLED_H_
#define _OLED_H_

#include <stdint.h>
#include <stdbool.h>

extern const uint8_t ASCII_0806[][6];
extern const uint8_t ASCII_1206[][12];

extern const uint8_t GBK_16[][32];

extern const uint8_t BMP_TESETLOGO[];

extern const uint8_t BMP_BTLOGO[];
extern const uint8_t BMP_BTLOGO_OFF[];
extern const uint8_t BMP_USB_LOGO[];

extern uint8_t OLED_GRAM[8][128]; // 128*64dpi

bool oledInit(void);

// Refresh in lines(0~7): start <= end
void oledRefresh(uint8_t sl, uint8_t el);


// Clear in lines(0~7)
void oledClear(uint8_t sl, uint8_t el);

// Draw line in columns(0~63)
void oledDrawLine(uint8_t y);

void oledShowStr(uint8_t x, uint8_t y, const char *str);

void oledShowGBKs(uint8_t x, uint8_t y, uint8_t sid, uint8_t eid);

// Show PIC(width:x+w<128, height:y+h<64, pic:w*h)
void oledShowBMP(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *pic);

#endif
