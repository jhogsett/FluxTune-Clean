#include "led_driver.h"

#ifndef NATIVE_BUILD

LEDDriver::LEDDriver(int pin, int count) : _pin(pin), _count(count) {
#ifdef ARDUINO_AVR_NANO_EVERY
    _strip = new Adafruit_NeoPixel(count, pin, NEO_GRB + NEO_KHZ800);
#else
    // For PololuLedStrip, we'll use a runtime check since it's template-based
    _strip_ptr = nullptr;  // Will be initialized in init() based on pin
#endif
}

void LEDDriver::init() {
#ifdef ARDUINO_AVR_NANO_EVERY
    if (_strip) {
        _strip->begin();
        _strip->clear();
        _strip->show();
    }
#else
    // PololuLedStrip doesn't need explicit initialization
    // The template-based approach handles this automatically
#endif
}

void LEDDriver::write(led_color* colors, int count) {
#ifdef ARDUINO_AVR_NANO_EVERY
    if (_strip && count <= _count) {
        for (int i = 0; i < count; i++) {
            _strip->setPixelColor(i, _strip->Color(colors[i].red, colors[i].green, colors[i].blue));
        }
        _strip->show();
    }
#else
    // For PololuLedStrip, we need to convert led_color to rgb_color
    // and use the global ledStrip instance (template-based, pin 12)
    if (_pin == 12 && count <= _count) {
        extern void write_led_strip_pin12(led_color* colors, int count);
        write_led_strip_pin12(colors, count);
    }
#endif
}

void LEDDriver::clear() {
#ifdef ARDUINO_AVR_NANO_EVERY
    if (_strip) {
        _strip->clear();
        _strip->show();
    }
#else
    // Clear using all-black colors
    led_color black_colors[_count];
    for (int i = 0; i < _count; i++) {
        black_colors[i] = led_color(0, 0, 0);
    }
    write(black_colors, _count);
#endif
}

#endif // NATIVE_BUILD
