#include "signal_meter.h"
#include "hardware.h"

#ifdef ENABLE_LOGARITHMIC_S_METER
#ifndef NATIVE_BUILD
#include <math.h>  // For log2f() function
#else
#include <cmath>   // For log2f() function in native builds
#endif
#endif

#ifndef NATIVE_BUILD
extern PololuLedStrip<12> ledStrip;  // Defined in main.cpp
extern int option_contrast;         // Defined in main.cpp (matches saved_data.cpp type)
#endif

// Color definitions: Green → Yellow → Red (like analog signal meters)
#ifndef NATIVE_BUILD
rgb_color SignalMeter::_led_colors[LED_COUNT] = {
    { 0, 15, 0 },   // Green
    { 0, 15, 0 },   // Green
    { 0, 15, 0 },   // Green
    { 0, 15, 0 },   // Green
    { 15, 15, 0 },  // Yellow
    { 15, 15, 0 },  // Yellow
    { 15, 0, 0 }    // Red
};
#endif

SignalMeter::SignalMeter()
{
    _accumulator = 0;
    _current_strength = 0;
    _last_decay_time = 0;
    _panel_led_accumulator = 0;
    _flashlight_mode = false;
    _flashlight_brightness = 0;
}

void SignalMeter::init()
{
    clear();
    _panel_led_accumulator = 0;
#ifndef NATIVE_BUILD
    _last_decay_time = millis();
#endif
}

void SignalMeter::add_charge(int charge_amount)
{    // Add charge pulse to accumulator (like electrical charge into capacitor)
    if (charge_amount < 0) {
        // Negative charge: treat as panel LED lock pulse
        int abs_charge = -charge_amount;
        _panel_led_accumulator += abs_charge; // Add absolute value
        if (_panel_led_accumulator > PANEL_LED_MAX_ACCUMULATOR) {
            _panel_led_accumulator = PANEL_LED_MAX_ACCUMULATOR;
        }
        charge_amount = abs_charge; // Also update main signal meter as if it was a regular pulse
    }
    _accumulator += charge_amount;
    // Clamp to maximum
    if (_accumulator > MAX_ACCUMULATOR) {
        _accumulator = MAX_ACCUMULATOR;
    }
    // Update display strength based on accumulator
    _current_strength = (_accumulator * 255) / MAX_ACCUMULATOR;
    write_leds();
}

void SignalMeter::update(unsigned long current_time)
{
    // Apply time-based decay (like capacitor discharging)
    if (current_time - _last_decay_time >= DECAY_INTERVAL) {
        if (_accumulator > 0) {
            _accumulator -= DECAY_RATE;
            if (_accumulator < 0) _accumulator = 0;
            // Update display strength
            _current_strength = (_accumulator * 255) / MAX_ACCUMULATOR;
            write_leds();
        }
        // Decay panel LED accumulator
        if (_panel_led_accumulator > 0) {
            _panel_led_accumulator -= PANEL_LED_DECAY_RATE;
            if (_panel_led_accumulator < 0) _panel_led_accumulator = 0;
        }
        _last_decay_time = current_time;
    }
}

void SignalMeter::update_signal_strength(int strength)
{
    // Legacy method - convert signal strength to charge pulses
    // This maintains backward compatibility while using the new charge system
    if (strength > 0) {
        // Convert strength to appropriate charge amount (reduced for better balance)
        // Higher signal strength = more charge pulses

        int charge_pulses = (strength * 2) / 40;  // Convert 0-255 to 0-12 charge pulses (reduced from 25)

        // for (int i = 0; i < charge_pulses; i++) {
        //     add_charge();  // Use default charge amount (now 6 instead of 10)
        // }
        add_charge(DEFAULT_CHARGE * charge_pulses);
    }
}

void SignalMeter::clear()
{
    _accumulator = 0;
    _current_strength = 0;
    _panel_led_accumulator = 0;
    write_leds();
}

void SignalMeter::clear_panel_led()
{
    _panel_led_accumulator = 0;
}

void SignalMeter::set_flashlight_mode(int brightness)
{
    _flashlight_mode = true;
    _flashlight_brightness = brightness;
    if (brightness < 0) _flashlight_brightness = 0;
    if (brightness > 255) _flashlight_brightness = 255;
    
    // Update LEDs immediately
    write_leds();
}

void SignalMeter::clear_flashlight_mode()
{
    _flashlight_mode = false;
    _flashlight_brightness = 0;
    
    // Update LEDs immediately to return to normal operation
    write_leds();
}

void SignalMeter::write_leds()
{
#ifndef NATIVE_BUILD
    if (_flashlight_mode) {
        // Flashlight mode: set all LEDs to white at specified brightness
        // White is created using RGB mix since these are RGB LEDs, not RGBW

        int white_brightness = _flashlight_brightness; // / SIGNAL_METER_BRIGHTNESS_DIVISOR;
        
        for (int i = 0; i < LED_COUNT; i++) {
            _led_buffer[i].red = white_brightness;
            _led_buffer[i].green = white_brightness;
            _led_buffer[i].blue = white_brightness;
        }
    } else {
        // Normal signal meter mode
        
#ifdef ENABLE_LOGARITHMIC_S_METER
        // Apply square root scaling for S-meter-like behavior
        // This compresses strong signals while keeping good sensitivity for weak signals
        // More balanced than pure logarithmic scaling
        int display_strength;
        if (_current_strength > 0) {
            // Square root scaling: sqrt(x) * scale_factor
            // Provides compression without the extreme behavior of logarithmic
            float sqrt_value = sqrtf((float)_current_strength);
            display_strength = (int)(sqrt_value * 16.0f);  // Scale to use 0-255 range
            if (display_strength > 255) display_strength = 255;
        } else {
            display_strength = 0;
        }
#else
        // Linear scaling (original behavior)
        int display_strength = _current_strength;
#endif
        
        // Convert strength to LED display
        int sample = display_strength * 2;  // Scale to 0-510
        int on_leds = (sample / 73) + 1;     // How many full LEDs to light
        int remain = ((sample % 73) * 16) / 73;  // Brightness of partial LED
        
        // Ensure we don't exceed LED count
        if (on_leds > LED_COUNT) on_leds = LED_COUNT;
        if (on_leds < 0) on_leds = 0;
        
        // Copy base colors for lit LEDs
        memcpy(_led_buffer, _led_colors, on_leds * sizeof(rgb_color));
        
        // Clear remaining LEDs
        memset(_led_buffer + on_leds, 0, (LED_COUNT - on_leds) * sizeof(rgb_color));
        
        // Apply partial brightness to last LED if needed
        if (on_leds > 0) {
            _led_buffer[on_leds-1].red = (_led_buffer[on_leds-1].red * remain) / 16;
            _led_buffer[on_leds-1].green = (_led_buffer[on_leds-1].green * remain) / 16;
            _led_buffer[on_leds-1].blue = (_led_buffer[on_leds-1].blue * remain) / 16;
        }
          // Apply contrast adjustment and device variant scaling
        for (int i = 0; i < LED_COUNT; i++) {
            _led_buffer[i].red = (_led_buffer[i].red * option_contrast) / SIGNAL_METER_BRIGHTNESS_DIVISOR;
            _led_buffer[i].green = (_led_buffer[i].green * option_contrast) / SIGNAL_METER_BRIGHTNESS_DIVISOR;
            _led_buffer[i].blue = (_led_buffer[i].blue * option_contrast) / SIGNAL_METER_BRIGHTNESS_DIVISOR;
        }
    }
    
    // Write to hardware
    ledStrip.write(_led_buffer, LED_COUNT);
#endif
}
