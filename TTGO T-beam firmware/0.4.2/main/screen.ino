
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplay.h"
#include "images.h"
#include "fonts.h"

#include <qrcode.h>
#include "SSD1306.h" // For QR code's use of OLED

#define SCREEN_HEADER_HEIGHT    14

SSD1306Wire * display;
uint8_t _screen_line = SCREEN_HEADER_HEIGHT - 1;

SSD1306 displayQR(0x3c, SDA, SCL);
//QRcode qrcode (&displayQR);

void _screen_header() {
    char buffer[20];

    // Message COUNT
    snprintf(buffer, sizeof(buffer), "#%03d", get_count() % 1000);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 2, buffer);

    // Datetime (if the axp192 PMIC is present, alternate between powerstats and time)
    if(AXP192_FOUND && millis()%8000 < 3000){
        // Possibly remove following line if it gives compile error
        //snprintf(buffer, sizeof(buffer), "%.1fV %.0fmA", AXP.getBattVoltage()/1000, AXP.getBattChargeCurrent() - AXP.getBattDischargeCurrent());

    } else {
        gps_time(buffer, sizeof(buffer));
    }
    
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(display->getWidth()/2, 2, buffer);

    // Satellite COUNT
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->drawString(display->getWidth() - SATELLITE_IMAGE_WIDTH - 4, 2, itoa(gps_sats(), buffer, 10));
    display->drawXbm(display->getWidth() - SATELLITE_IMAGE_WIDTH, 0, SATELLITE_IMAGE_WIDTH, SATELLITE_IMAGE_HEIGHT, SATELLITE_IMAGE);
}

void screen_show_logo() {
    uint8_t x = (display->getWidth() - TTN_IMAGE_WIDTH) / 2;
    uint8_t y = SCREEN_HEADER_HEIGHT + (display->getHeight() - SCREEN_HEADER_HEIGHT - TTN_IMAGE_HEIGHT) / 2 + 1;
    display->drawXbm(x, y, TTN_IMAGE_WIDTH, TTN_IMAGE_HEIGHT, TTN_IMAGE);
}

void screen_show_qrcode() {
    //QRcode qrcode (&display);

    //qrcode.init();
    //qrcode.create("https://telokanda.yensesa.com/?cntrt=wxballoons2t");
}

void screen_off() {
    display->displayOff();
}

void screen_on() {
    display->displayOn();
}

void screen_clear() {
    display->clear();
}

void screen_print(const char * text, uint8_t x, uint8_t y, uint8_t alignment) {
    DEBUG_MSG(text);
    display->setTextAlignment((OLEDDISPLAY_TEXT_ALIGNMENT) alignment);
    display->drawString(x, y, text);
}

void screen_print(const char * text, uint8_t x, uint8_t y) {
    screen_print(text, x, y, TEXT_ALIGN_LEFT);
}

void screen_print(const char * text) {

    display->print(text);
    if (_screen_line + 8 > display->getHeight()) {
      // scroll
    }
    _screen_line += 8;
    screen_loop(); 
}

void screen_update() {
    display->display();
}

void screen_setup() {
    // Display instance
    display = new SSD1306Wire(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
    display->init();
    display->flipScreenVertically();
    display->setFont(Custom_ArialMT_Plain_10);

    // Scroll buffer
    display->setLogBuffer(5, 30);
}

void screen_loop() {

    display->clear();
    _screen_header();
    display->drawLogBuffer(0, SCREEN_HEADER_HEIGHT);
    display->display();
}
