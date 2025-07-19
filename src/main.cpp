#include "station_config.h"

#include <Arduino.h>
#include <Wire.h>
#include <MD_AD9833.h>
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>

#include "displays.h"
#include "hardware.h"
#include "leds.h"
#include "saved_data.h"
#include "seeding.h"
#include "utils.h"
#include "signal_meter.h"
#include "station_config.h"
#include "station_manager.h"

#include "encoder_handler.h"

#include "vfo.h"
#include "vfo_tuner.h"
#include "event_dispatcher.h"

#include "contrast.h"
#include "contrast_handler.h"
#include "bfo.h"
#include "bfo_handler.h"
#include "flashlight.h"
#include "flashlight_handler.h"

#include "wavegen.h"

#ifdef ENABLE_MORSE_STATION
#include "sim_station.h"
#endif

#ifdef ENABLE_NUMBERS_STATION
#include "sim_numbers.h"
#endif

#ifdef ENABLE_RTTY_STATION
#include "sim_rtty.h"
#endif

#ifdef ENABLE_PAGER_STATION
#include "sim_pager.h"
#endif

#ifdef ENABLE_PAGER2_STATION
#include "sim_pager2.h"
#endif

#ifdef ENABLE_JAMMER_STATION
#include "sim_jammer.h"
#endif
#ifdef ENABLE_TEST_STATION
#include "sim_test.h"
#endif

#include "async_morse.h"

#include "wave_gen_pool.h"

#ifdef USE_EEPROM_TABLES
#include "eeprom_tables.h"
#endif

#include "signal_meter.h"

// ============================================================================
// BRANDING MODE FOR PRODUCT PHOTOGRAPHY
// Comment out this #define to disable branding mode and save Flash memory
// ============================================================================
#define ENABLE_BRANDING_MODE  // OPTIMIZATION: Disabled by default to save Flash

// Create an ledStrip object and specify the pin it will use.
// Now using Adafruit NeoPixel for both platforms
// PololuLedStrip<12> ledStrip;

// #define LED_COUNT 7




#define CLKA 3
#define DTA 2
#define SWA 4

#define CLKB 6
#define DTB 5
#define SWB 7

#define PULSES_PER_DETENT 2

// Display handling
// show a display string for 700ms before beginning scrolling for ease of reading
#define DISPLAY_SHOW_TIME 800  // Restored to original value
// scroll the display every 90ms for ease of reading
#define DISPLAY_SCROLL_TIME 70
// scroll flipped options every 100ms
#define OPTION_FLIP_SCROLL_TIME 100

EncoderHandler encoder_handlerA(0, CLKA, DTA, SWA, PULSES_PER_DETENT);
EncoderHandler encoder_handlerB(1, CLKB, DTB, SWB, PULSES_PER_DETENT);


// Pins for SPI comm with the AD9833 IC
const byte PIN_DATA = 11;  ///< SPI Data pin number
const byte PIN_CLK = 13;  	///< SPI Clock pin number
const byte PIN_FSYNC1 = 8; ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC2 = 14;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC3 = 15;  ///< SPI Load pin number (FSYNC in AD9833 usage)
const byte PIN_FSYNC4 = 16;  ///< SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833 AD1(PIN_DATA, PIN_CLK, PIN_FSYNC1); // Arbitrary SPI pins
MD_AD9833 AD2(PIN_DATA, PIN_CLK, PIN_FSYNC2); // Arbitrary SPI pins
MD_AD9833 AD3(PIN_DATA, PIN_CLK, PIN_FSYNC3); // Arbitrary SPI pins
MD_AD9833 AD4(PIN_DATA, PIN_CLK, PIN_FSYNC4); // Arbitrary SPI pins

WaveGen wavegen1(&AD1);
WaveGen wavegen2(&AD2);
WaveGen wavegen3(&AD3);
WaveGen wavegen4(&AD4);

WaveGen *wavegens[4] = {&wavegen1, &wavegen2, &wavegen3, &wavegen4};
bool realizer_stats[4] = {false, false, false, false};
WaveGenPool wave_gen_pool(wavegens, realizer_stats, 4);

// Signal meter instance
SignalMeter signal_meter;

// ============================================================================
// STATION CONFIGURATION - Conditional compilation based on station_config.h
// ============================================================================
//
// *** MEMORY OPTIMIZATION: SHARED REALIZATION ARRAYS ***
//
// DESIGN PATTERN: Zero-copy array sharing between RealizationPool and StationManager
// - Single realizations[] array serves both managers (eliminates duplicate station_pool[] arrays)
// - StationManager constructor casts Realization* to SimTransmitter* (safe due to dual inheritance)
// - Memory savings: 8-168 bytes per configuration depending on station count
//
// INHERITANCE REQUIREMENT: All station classes MUST inherit from BOTH:
//   - SimTransmitter (for StationManager compatibility)
//   - Realization (for RealizationPool compatibility)
//
// *** CRITICAL ARRAY SYNCHRONIZATION REQUIREMENTS ***
//
// When adding/removing stations or creating new configurations, you MUST update:
//
// 1. realizations[SIZE] array declaration (match actual station count)
// 2. realization_stats[SIZE] array in conditional #ifdef block (lines ~590-602)
// 3. RealizationPool constructor parameter (lines ~610-620)
// 4. StationManager constructor parameter (lines ~570-580)
//
// Example: CONFIG_MIXED_STATIONS currently has 2 stations:
//   - realizations[2] (defined in config sections below)
//   - realization_stats[2] (defined around line 597)
//   - RealizationPool(..., 2) and StationManager(realizations, 2)
//
// *** RESTART BUG WARNING ***
// Mismatched array sizes cause continuous Arduino restarts!
// Always verify station count matches ALL array declarations!
//
// ============================================================================

#ifdef CONFIG_MIXED_STATIONS
// Testing: CW + SimPager2 (dual wave generator) for dual-tone breakthrough testing
#ifdef ENABLE_MORSE_STATION
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7007000.0, 8);   // SLOW: 8 WPM to hold generators longer
#endif
#ifdef ENABLE_NUMBERS_STATION
SimNumbers numbers_station1(&wave_gen_pool, &signal_meter, 7002700.0, 18);
#endif
#ifdef ENABLE_RTTY_STATION
SimRTTY rtty_station1(&wave_gen_pool, &signal_meter, 14004100.0);
#endif
#ifdef ENABLE_JAMMER_STATION
SimJammer jammer_station1(&wave_gen_pool);
#endif
#ifdef ENABLE_PAGER_STATION
SimPager pager_station1(&wave_gen_pool, &signal_meter, 146800000.0);
#endif

#ifdef ENABLE_PAGER2_STATION
SimPager2 pager2_station1(&wave_gen_pool, &signal_meter, 7000000.0);  // Testing dual wave generator - moved well above other stations
#endif

// Shared array - serves as both station pool and realizations  
Realization *realizations[2] = {
    &cw_station1,       // [0] - CW station  
    &pager2_station1    // [1] - Dual-tone pager (testing)
};
#endif

#ifdef CONFIG_CW_CLUSTER
// LISTENING PLEASURE: Four CW stations clustered in low 40m band
// Frequencies chosen to often overlap in reception for realistic band activity
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7002000.0, 12);  // 12 WPM
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7003500.0, 16);  // 16 WPM  
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7004200.0, 18);  // 18 WPM
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7005800.0, 22);  // 22 WPM

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4
};
#endif

#ifdef CONFIG_FOUR_CW
// TEST: Four CW stations with different speeds
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7002000.0, 11);
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7003000.0, 15);
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7004000.0, 20);
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7005000.0, 25);

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4
};
#endif

#ifdef CONFIG_FIVE_CW
// Four CW stations with different speeds and realistic fist qualities for Field Day
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7001500.0, 31, 10);   // Advanced/Extra portion, fast precise sender
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7002200.0, 19, 50);   // Advanced/Extra portion, slower tired sender
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7002900.0, 11, 95);   // Novice/General portion, novice first timer  
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7003600.0, 15, 40);   // Novice/General portion, experienced new ham
SimStation cw_station5(&wave_gen_pool, &signal_meter, 7004300.0, 25, 80);   // Novice/General portion, experienced tired ham

// Shared array - serves as both station pool and realizations
Realization *realizations[5] = {
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4,
    &cw_station5
};
#endif

#ifdef CONFIG_FIVE_CW_RESOURCE_TEST
// Five CW stations with only 4 wave generators - resource contention test
// Different frequencies spread across 40m band for easy identification
// First 4 stations at 30 WPM for fast recycling, station 5 at 13 WPM for contrast
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7001500.0, 30, 10);   // Station 1: 30 WPM, slight fist variation
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7002200.0, 30, 20);   // Station 2: 30 WPM, moderate fist variation
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7002900.0, 30, 30);   // Station 3: 30 WPM, noticeable fist variation
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7003600.0, 30, 15);   // Station 4: 30 WPM, slight fist variation
SimStation cw_station5(&wave_gen_pool, &signal_meter, 7004300.0, 13, 25);   // Station 5: 13 WPM, moderate fist variation

SimTransmitter *station_pool[5] = {  // 5 stations but only 4 wave generators available
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4,
    &cw_station5
};

Realization *realizations[5] = {  // 5 realizations competing for 4 resources
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4,
    &cw_station5
};
#endif

#ifdef CONFIG_TEN_CW
// STRESS TEST: 21 stations total for Arduino Nano Every comprehensive testing
// 10 CW stations spread across 40m band with varying speeds and fist qualities
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7001000.0, 31, 10);   // Fast precise sender
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7001500.0, 19, 50);   // Slower tired sender
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7002000.0, 11, 95);   // Novice first timer  
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7002500.0, 15, 40);   // Experienced new ham
SimStation cw_station5(&wave_gen_pool, &signal_meter, 7003000.0, 25, 80);   // Experienced tired ham
SimStation cw_station6(&wave_gen_pool, &signal_meter, 7003500.0, 22, 30);   // Contest station
SimStation cw_station7(&wave_gen_pool, &signal_meter, 7004000.0, 18, 60);   // Casual operator
SimStation cw_station8(&wave_gen_pool, &signal_meter, 7004500.0, 28, 20);   // Expert DXer
SimStation cw_station9(&wave_gen_pool, &signal_meter, 7005000.0, 13, 70);   // QRP enthusiast
SimStation cw_station10(&wave_gen_pool, &signal_meter, 7005500.0, 16, 45);  // Ragchewer

// 5 Numbers stations above CW in 40m band
SimNumbers numbers_station1(&wave_gen_pool, &signal_meter, 7006000.0, 12);  // Standard numbers
SimNumbers numbers_station2(&wave_gen_pool, &signal_meter, 7007000.0, 15);  // Faster numbers
SimNumbers numbers_station3(&wave_gen_pool, &signal_meter, 7008000.0, 18);  // Quick numbers
SimNumbers numbers_station4(&wave_gen_pool, &signal_meter, 7009000.0, 22);  // Rapid numbers
SimNumbers numbers_station5(&wave_gen_pool, &signal_meter, 7010000.0, 10);  // Slow numbers

// 4 RTTY stations on 20m band
SimRTTY rtty_station1(&wave_gen_pool, &signal_meter, 14002000.0);          // RTTY station 1
SimRTTY rtty_station2(&wave_gen_pool, &signal_meter, 14004000.0);          // RTTY station 2
SimRTTY rtty_station3(&wave_gen_pool, &signal_meter, 14006000.0);          // RTTY station 3
SimRTTY rtty_station4(&wave_gen_pool, &signal_meter, 14008000.0);          // RTTY station 4

// 2 Pager stations on 2m band
SimPager pager_station1(&wave_gen_pool, &signal_meter, 146800000.0);       // Standard pager
SimPager pager_station2(&wave_gen_pool, &signal_meter, 146900000.0);       // Second pager

SimTransmitter *station_pool[21] = {  // 21 stations competing for 4 wave generators
    &cw_station1, &cw_station2, &cw_station3, &cw_station4, &cw_station5,
    &cw_station6, &cw_station7, &cw_station8, &cw_station9, &cw_station10,
    &numbers_station1, &numbers_station2, &numbers_station3, &numbers_station4, &numbers_station5,
    &rtty_station1, &rtty_station2, &rtty_station3, &rtty_station4,
    &pager_station1, &pager_station2
};

Realization *realizations[21] = {  // 21 realizations competing for 4 resources
    &cw_station1, &cw_station2, &cw_station3, &cw_station4, &cw_station5,
    &cw_station6, &cw_station7, &cw_station8, &cw_station9, &cw_station10,
    &numbers_station1, &numbers_station2, &numbers_station3, &numbers_station4, &numbers_station5,
    &rtty_station1, &rtty_station2, &rtty_station3, &rtty_station4,
    &pager_station1, &pager_station2
};
#endif

#ifdef CONFIG_FOUR_NUMBERS
// TEST: Four Numbers stations with different frequencies  
SimNumbers numbers_station1(&wave_gen_pool, &signal_meter, 7002700.0, 12);
SimNumbers numbers_station2(&wave_gen_pool, &signal_meter, 7003700.0, 15);
SimNumbers numbers_station3(&wave_gen_pool, &signal_meter, 7004700.0, 18);
SimNumbers numbers_station4(&wave_gen_pool, &signal_meter, 7005700.0, 22);

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &numbers_station1,
    &numbers_station2,
    &numbers_station3,
    &numbers_station4
};
#endif

#ifdef CONFIG_FOUR_PAGER
// TEST: Four Pager stations
SimPager pager_station1(&wave_gen_pool, &signal_meter, 7006000.0);
SimPager pager_station2(&wave_gen_pool, &signal_meter, 7007000.0);
SimPager pager_station3(&wave_gen_pool, &signal_meter, 7008000.0);
SimPager pager_station4(&wave_gen_pool, &signal_meter, 7009000.0);

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &pager_station1,
    &pager_station2,
    &pager_station3,
    &pager_station4
};
#endif

#ifdef CONFIG_FOUR_RTTY
// TEST: Four RTTY stations
SimRTTY rtty_station1(&wave_gen_pool, &signal_meter, 7004100.0);
SimRTTY rtty_station2(&wave_gen_pool, &signal_meter, 7005100.0);
SimRTTY rtty_station3(&wave_gen_pool, &signal_meter, 7006100.0);
SimRTTY rtty_station4(&wave_gen_pool, &signal_meter, 7007100.0);

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &rtty_station1,
    &rtty_station2,
    &rtty_station3,
    &rtty_station4
};
#endif

#ifdef CONFIG_FOUR_JAMMER
// TEST: Four Jammer stations for interference testing
SimJammer jammer_station1(&wave_gen_pool);
SimJammer jammer_station2(&wave_gen_pool);
SimJammer jammer_station3(&wave_gen_pool);
SimJammer jammer_station4(&wave_gen_pool);

// Shared array - serves as both station pool and realizations
Realization *realizations[4] = {
    &jammer_station1,
    &jammer_station2,
    &jammer_station3,
    &jammer_station4
};
#endif

#ifdef CONFIG_MINIMAL_CW
// MINIMAL: Single CW station for fist quality testing
// Change the fist quality value here to test different levels:
//   0 = Perfect timing (mechanical precision)
//   5 = Very slight variations (should work)
//  10 = Low fist quality (should work) 
//  40 = Moderate fist quality (current Field Day "novice" setting)
//  80 = High fist quality (current Field Day "tired operator" setting)
// 255 = Maximum bad fist (extreme case)

SimStation cw_station1(&wave_gen_pool, &signal_meter, 7000000.0, 25, 25);  // Field Day Station test: MAXIMUM fist quality for testing - MOVED TO 7.000 MHz to match VFO A default!

// Shared array - serves as both station pool and realizations
Realization *realizations[1] = {  // Back to single station for minimal config
    &cw_station1
};
#endif

#ifdef CONFIG_DEV_LOW_RAM
// DEVELOPMENT: Low RAM configuration - only essential stations for development
#ifdef ENABLE_MORSE_STATION
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7002000.0, 11);
#endif
#ifdef ENABLE_NUMBERS_STATION
SimNumbers numbers_station1(&wave_gen_pool, &signal_meter, 7002700.0, 18);
#endif
#ifdef ENABLE_TEST_STATION
SimTest test_station(&wave_gen_pool, &signal_meter, 7005000.0, 10.0, 440.0, 560.0);  // 10 Hz toggle, 440 Hz and 560 Hz tones
#endif

SimTransmitter *station_pool[3] = {  // *** WARNING: CONFIG_DEV_LOW_RAM may have different station count! ***
                                  // Verify actual station count matches array size [3]!
#ifdef ENABLE_MORSE_STATION
    &cw_station1,
#endif
#ifdef ENABLE_NUMBERS_STATION
    &numbers_station1,
#endif
#ifdef ENABLE_TEST_STATION
    &test_station
#endif
};

Realization *realizations[3] = {  // *** WARNING: CONFIG_DEV_LOW_RAM array size must match station_pool[3] above! ***
#ifdef ENABLE_MORSE_STATION
    &cw_station1,
#endif
#ifdef ENABLE_NUMBERS_STATION
    &numbers_station1,
#endif
#ifdef ENABLE_TEST_STATION
    &test_station
#endif
};
#endif

#ifdef CONFIG_FILE_PILE_UP
// Five CW stations simulating pile-up activity on 40m
// Each station has its own callsign but all are calling for a realistic pile-up
// Different speeds and fist qualities to simulate various operators
// Frequencies spaced 1 kHz apart around 7.002 MHz for pipelining testing
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7002500.0, 28, 15);   // JA1ABC: Fast confident contest operator
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7001500.0, 22, 40);   // VK2DEF: Experienced DXer, slightly nervous  
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7003500.0, 16, 80);   // W3GHI: General class, first big DX contact
// SimStation cw_station4(&wave_gen_pool, &signal_meter, 7004500.0, 25, 25);   // DL4JKL: European, good operator but tired
// SimStation cw_station5(&wave_gen_pool, &signal_meter, 7000500.0, 19, 60);   // VE5MNO: Canadian, rusty on CW but determined

SimTransmitter *station_pool[3] = {  // *** WARNING: CONFIG_FILE_PILE_UP currently has 3 stations, but could have 5! ***
                                  // Verify actual station count matches array size [3]!
                                  // If enabling all 5 pile-up stations, change to [5]!
    &cw_station1,
    &cw_station2, 
    &cw_station3
    // &cw_station4,
    // &cw_station5
};

Realization *realizations[3] = {  // *** WARNING: CONFIG_FILE_PILE_UP array size must match station_pool[3] above! ***
                              // If enabling all 5 pile-up stations, change to [5]!
    &cw_station1,
    &cw_station2,
    &cw_station3
    // &cw_station4,
    // &cw_station5
};
#endif

#ifdef CONFIG_FIVE_CW_RESOURCE_TEST
// Five CW stations with only 4 wave generators - resource contention test
// Different frequencies spread across 40m band for easy identification
// First 4 stations at 30 WPM for fast recycling, station 5 at 13 WPM for contrast
SimStation cw_station1(&wave_gen_pool, &signal_meter, 7001000.0, 30, 10);   // Station 1: 30 WPM, slight fist variation
SimStation cw_station2(&wave_gen_pool, &signal_meter, 7002000.0, 30, 20);   // Station 2: 30 WPM, moderate fist variation
SimStation cw_station3(&wave_gen_pool, &signal_meter, 7003000.0, 30, 30);   // Station 3: 30 WPM, noticeable fist variation
SimStation cw_station4(&wave_gen_pool, &signal_meter, 7004000.0, 30, 15);   // Station 4: 30 WPM, slight fist variation
SimStation cw_station5(&wave_gen_pool, &signal_meter, 7005000.0, 13, 25);   // Station 5: 13 WPM, moderate fist variation

SimTransmitter *station_pool[5] = {  // 5 stations but only 4 wave generators available
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4,
    &cw_station5
};

Realization *realizations[5] = {  // 5 realizations competing for 4 resources
    &cw_station1,
    &cw_station2,
    &cw_station3,
    &cw_station4,
    &cw_station5
};
#endif

#ifdef CONFIG_TEST_PERFORMANCE
// Performance test: Single test station cycling between two frequencies as fast as possible
SimTest test_station(&wave_gen_pool, &signal_meter, 7002000.0, 1000.0);  // Base at 7.002 MHz, +/- 1 kHz

SimTransmitter *station_pool[1] = {
    &test_station
};

Realization *realizations[1] = {
    &test_station
};
#endif

#ifdef CONFIG_PAGER2_TEST
// Test config with original SimPager to isolate if issue is config or SimPager2 class
SimPager pager_test(&wave_gen_pool, &signal_meter, 146800000.0);  // 2 meter pager frequency

SimTransmitter *station_pool[1] = {
    &pager_test
};

Realization *realizations[1] = {
    &pager_test
};
#endif

// ============================================================================
// REALIZATION POOL - Initialize with configured realizations
// ============================================================================

// Realization status array - sized based on configuration
#ifdef CONFIG_MINIMAL_CW
bool realization_stats[1] = {false};
#elif defined(CONFIG_TEST_PERFORMANCE)
bool realization_stats[1] = {false};  // Single test station
#elif defined(CONFIG_PAGER2_TEST)
bool realization_stats[1] = {false};  // Single dual-tone pager station
#elif defined(CONFIG_DEV_LOW_RAM)
bool realization_stats[3] = {false, false, false};
#elif defined(CONFIG_FIVE_CW_RESOURCE_TEST)
bool realization_stats[5] = {false, false, false, false, false};  // 5 stations for resource test
#elif defined(CONFIG_TEN_CW)
bool realization_stats[21] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};  // 21 stations for comprehensive Nano Every stress test
#elif defined(CONFIG_FILE_PILE_UP)
bool realization_stats[3] = {false, false, false};  // 3 stations for pile-up debug
#elif defined(CONFIG_MIXED_STATIONS)
bool realization_stats[2] = {false, false};  // *** ACTUAL STATION COUNT: 2 stations (cw + pager2) ***
#else
bool realization_stats[4] = {false, false, false, false};
#endif

#ifdef CONFIG_MINIMAL_CW
RealizationPool realization_pool(realizations, realization_stats, 1);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_TEST_PERFORMANCE)
RealizationPool realization_pool(realizations, realization_stats, 1);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_PAGER2_TEST)
RealizationPool realization_pool(realizations, realization_stats, 1);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_DEV_LOW_RAM)
RealizationPool realization_pool(realizations, realization_stats, 3);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_FIVE_CW_RESOURCE_TEST)
RealizationPool realization_pool(realizations, realization_stats, 5);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_TEN_CW)
RealizationPool realization_pool(realizations, realization_stats, 21);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_FILE_PILE_UP)
RealizationPool realization_pool(realizations, realization_stats, 3);  // *** CRITICAL: Count must match arrays above! ***
#elif defined(CONFIG_MIXED_STATIONS)
RealizationPool realization_pool(realizations, realization_stats, 2);  // *** CRITICAL: Count must match arrays above! ***
#else
RealizationPool realization_pool(realizations, realization_stats, 4);  // *** CRITICAL: Count must match arrays above! ***
#endif

// ============================================================================
// STATION MANAGER - Initialize with configured station pool
// ============================================================================
#ifdef CONFIG_MINIMAL_CW
StationManager station_manager(realizations, 1);
#elif defined(CONFIG_MIXED_STATIONS)
StationManager station_manager(realizations, 2);  // cw_station1 + pager2_station1
#elif defined(CONFIG_FOUR_CW) || defined(CONFIG_FOUR_NUMBERS) || defined(CONFIG_FOUR_PAGER) || defined(CONFIG_FOUR_RTTY) || defined(CONFIG_FOUR_JAMMER) || defined(CONFIG_CW_CLUSTER)
StationManager station_manager(realizations, 4);
#elif defined(CONFIG_FIVE_CW) || defined(CONFIG_FIVE_CW_RESOURCE_TEST)
StationManager station_manager(realizations, 5);
#elif defined(CONFIG_DEV_LOW_RAM) || defined(CONFIG_FILE_PILE_UP)
StationManager station_manager(station_pool, 3);
#elif defined(CONFIG_TEN_CW)
StationManager station_manager(station_pool, 21);
#elif defined(CONFIG_TEST_PERFORMANCE)
StationManager station_manager(station_pool, 1);
#else
StationManager station_manager(station_pool, 4);  // Default fallback
#endif

VFO vfoa("VFO A",   7000000.0, 10, &realization_pool);
VFO vfob("VFO B",  14000000.0, 10, &realization_pool);
VFO vfoc("VFO C", 146520000.0, 5000, &realization_pool);

Contrast contrast("Contrast");
BFO bfo("Offset");
Flashlight flashlight("Light");

VFO_Tuner tunera(&vfoa);
VFO_Tuner tunerb(&vfob);
VFO_Tuner tunerc(&vfoc);

ContrastHandler contrast_handler(&contrast);
BFOHandler bfo_handler(&bfo);
FlashlightHandler flashlight_handler(&flashlight);

ModeHandler *handlers1[3] = {&tunera, &tunerb, &tunerc};
ModeHandler *handlers3[3] = {&contrast_handler, &bfo_handler, &flashlight_handler};

EventDispatcher dispatcher1(handlers1, 3);
EventDispatcher dispatcher3(handlers3, 3);

EventDispatcher * dispatcher = &dispatcher1;
int current_dispatcher = 1;

#define APP_SIMRADIO 1
#define APP_SETTINGS 2

void setup_display(){
	Wire.begin();

	const byte display_brightnesses[] = {(unsigned char)option_contrast, (unsigned char)option_contrast};
	display.init(display_brightnesses);
	display.clear();
}

void setup_signal_meter(){
	signal_meter.init();
}

void setup_leds(){
	for(byte i = FIRST_LED; i <= LAST_LED; i++){
		pinMode(i, OUTPUT);
		digitalWrite(i, LOW);
	}
	// unsigned long time = millis();
	// panel_leds.begin(time, LEDHandler::STYLE_RANDOM, DEFAULT_PANEL_LEDS_SHOW_TIME, DEFAULT_PANEL_LEDS_BLANK_TIME);
	// button_leds.begin(time, LEDHandler::STYLE_BLANKING, DEFAULT_BUTTON_LEDS_SHOW_TIME, DEFAULT_BUTTON_LEDS_BLANK_TIME);
	// all_leds.begin(time, LEDHandler::STYLE_RANDOM | LEDHandler::STYLE_BLANKING | LEDHandler::STYLE_MIRROR, DEFAULT_ALL_LEDS_SHOW_TIME, DEFAULT_ALL_LEDS_BLANK_TIME);
}

void setup_buttons(){
	// for(byte i = 0; i < NUM_BUTTONS; i++){
	// 	pinMode(i + FIRST_BUTTON, 0x03); // INPUT_PULLDOWN ?!
	// 	button_states[i] = false;
	// }
}

void setup(){
	Serial.begin(115200);
	
	randomizer.randomize();

#ifdef USE_EEPROM_TABLES
	// Initialize EEPROM tables if enabled
	if (!eeprom_tables_init()) {
		Serial.println("WARNING: EEPROM tables not loaded!");
		Serial.println("Run the eeprom_table_loader sketch first.");
		// Continue anyway - tables will fall back to Flash or fail gracefully
	}
#endif

	load_save_data();
	setup_leds();
	setup_display();
	setup_signal_meter();

	AD1.begin();
	AD1.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD1.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD1.setMode(MD_AD9833::MODE_SINE);

	AD2.begin();
	AD2.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD2.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD2.setMode(MD_AD9833::MODE_SINE);

	AD3.begin();
	AD3.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD3.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD3.setMode(MD_AD9833::MODE_SINE);
	
	AD4.begin();
	AD4.setFrequency((MD_AD9833::channel_t)0, 0.1);
	AD4.setFrequency((MD_AD9833::channel_t)1, 0.1);
	AD4.setMode(MD_AD9833::MODE_SINE);

	// Initialize StationManager with dynamic pipelining
	station_manager.enableDynamicPipelining(true);
	station_manager.setupPipeline(7000000); // Start with VFO A frequency
}

#ifdef ENABLE_BRANDING_MODE
// ============================================================================
// BRANDING MODE - Easter egg for product photography
// Activates when encoder A button is pressed during startup
// Sets signal meter to full strength and lights panel LEDs at max brightness
// ============================================================================
void activate_branding_mode() {
	Adafruit_NeoPixel* led_strip = nullptr;

	// Keep display showing "FluxTune" from previous code - perfect for branding photos!
      // Directly set signal meter LEDs to 4x brightness (bypass dynamic system)
	// rgb_color full_colors[LED_COUNT] = 
#ifdef DEVICE_VARIANT_RED_DISPLAY
// Color values for NeoPixel (red, green, blue ordering)
static const uint32_t BRAND_COLORS[SignalMeter::LED_COUNT] = {
    0x0F0000,   // Red
    0x0F0700,   // Orange
    0x0F0F00,   // Yellow
    0x000F00,   // Green
    0x000F0F,   // Cyan
    0x00000F,   // Blue
    0x07000F    // Purple
};
#else
// Color values for NeoPixel (red, green, blue ordering)
static const uint32_t BRAND_COLORS[SignalMeter::LED_COUNT] = {
    0x000F00,   // Green
    0x000F00,   // Green  
    0x000F00,   // Green
    0x000F00,   // Green
    0x0F0F00,   // Yellow
    0x0F0F00,   // Yellow
    0x0F0000,    // Red
};
#endif

	led_strip = new Adafruit_NeoPixel(SignalMeter::LED_COUNT, SIGNAL_METER_PIN, NEO_GRB + NEO_KHZ800);
	led_strip->begin();
	led_strip->clear();
	led_strip->show();
	
	// Enter infinite loop for photography - device stays in perfect display state
	while(true) {
		for(int i = 0; i < SignalMeter::LED_COUNT; i++){
			uint32_t color = BRAND_COLORS[i];
			// uint8_t r = (color >> 16) & 0xFF;
			// uint8_t g = (color >> 8) & 0xFF;
			// uint8_t b = color & 0xFF;
			// led_strip->setPixelColor(i, led_strip->Color(r, g, b));
			led_strip->setPixelColor(i, color);
		}
		led_strip->show();

		// Keep signal meter LEDs at full brightness (handled by SignalMeter class now)
        // Keep both panel LEDs at 4x maximum brightness
        analogWrite(WHITE_PANEL_LED, (PANEL_LOCK_LED_FULL_BRIGHTNESS * 4) / PANEL_LED_BRIGHTNESS_DIVISOR);
        analogWrite(BLUE_PANEL_LED, (PANEL_LOCK_LED_FULL_BRIGHTNESS * 4) / PANEL_LED_BRIGHTNESS_DIVISOR);
        
        // Small delay to prevent overwhelming the processor
        delay(100);
    }
}
#endif

EventDispatcher * set_application(int application, HT16K33Disp *display){
	EventDispatcher *dispatcher;
	char *title;	switch(application){
		case APP_SIMRADIO:
			dispatcher = &dispatcher1;
			current_dispatcher = APP_SIMRADIO;
			title = (FSTR("SimRadio"));
		break;

		case APP_SETTINGS:
			dispatcher = &dispatcher3;
			current_dispatcher = APP_SETTINGS;
			title = (FSTR("Settings"));
		break;	}
	display->scroll_string(title, DISPLAY_SHOW_TIME, DISPLAY_SCROLL_TIME);

	// we don't need this after removing the "Wave Gen" application that overtook the wave generators
	// // Mark hardware state as dirty when switching to SimRadio  
	// // This ensures audio resumes properly after application switches
	// if(application == APP_SIMRADIO) {
	// 	realization_pool.mark_dirty();
	// }
	
	dispatcher->set_mode(display, 0);
	
	// Force realization update when switching to SimRadio to ensure audio resumes immediately
	if(application == APP_SIMRADIO) {
		dispatcher->update_realization();
	}

	// // empty outstanding events
	// encoder_handlerA.changed();
	// encoder_handlerB.changed();
	// encoder_handlerA.pressed();
	// encoder_handlerA.long_pressed();
	// encoder_handlerB.pressed();
	// encoder_handlerB.long_pressed();



	// display.scroll_string(title, DISPLAY_SHOW_TIME, DISPLAY_SCROLL_TIME);
	// dispatcher->set_mode(&display, 0);

	return dispatcher;
}

void purge_events(){
	// Clear all pending encoder events
	while(encoder_handlerA.changed() || encoder_handlerB.changed() || 
	      encoder_handlerA.pressed() || encoder_handlerA.long_pressed() ||
	      encoder_handlerB.pressed() || encoder_handlerB.long_pressed());
}

void loop()
{
    display.scroll_string(FSTR("FLuXTuNE"), DISPLAY_SHOW_TIME, DISPLAY_SCROLL_TIME);

#ifdef ENABLE_BRANDING_MODE
    // BRANDING MODE EASTER EGG - Check if encoder A button is pressed during startup
    // Pin 4 (SWA) goes LOW when button is pressed
    if (digitalRead(SWA) == LOW) {
        activate_branding_mode();  // Never returns - infinite loop for photography
    }
#endif

    unsigned long time = millis();
    
    // ============================================================================
    // INITIALIZE STATIONS - Start stations based on configuration
    // ============================================================================
	
#ifdef CONFIG_MIXED_STATIONS
	// Initialize SimPager2 FIRST to test dual generator acquisition without resource pressure
#ifdef ENABLE_PAGER2_STATION
	pager2_station1.begin(time + random(1000));
	pager2_station1.set_station_state(AUDIBLE);
#endif

#ifdef ENABLE_MORSE_STATION
	cw_station1.begin(time + random(3000));
	cw_station1.set_station_state(AUDIBLE);
#endif
#endif

#ifdef CONFIG_CW_CLUSTER
	// Initialize CW cluster for listening pleasure
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);
	
	cw_station2.begin(time + random(2000));
	cw_station2.set_station_state(AUDIBLE);
	
	cw_station3.begin(time + random(3000));
	cw_station3.set_station_state(AUDIBLE);
	
	// cw_station4.begin(time + random(4000));
	// cw_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FOUR_CW
	// Initialize four CW test stations
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);
	
	cw_station2.begin(time + random(2000));
	cw_station2.set_station_state(AUDIBLE);
	
	cw_station3.begin(time + random(3000));
	cw_station3.set_station_state(AUDIBLE);
	
	// cw_station4.begin(time + random(4000));
	// cw_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FIVE_CW
	// Initialize four CW test stations
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);
	
	cw_station2.begin(time + random(2000));
	cw_station2.set_station_state(AUDIBLE);
	
	cw_station3.begin(time + random(3000));
	cw_station3.set_station_state(AUDIBLE);
	
	cw_station4.begin(time + random(4000));
	cw_station4.set_station_state(AUDIBLE);

	cw_station5.begin(time + random(5000));
	cw_station5.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_TEN_CW
	// Initialize 21 stations for comprehensive Nano Every stress testing
	
	// Initialize 10 CW stations
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);

	cw_station2.begin(time + random(2000));
	cw_station2.set_station_state(AUDIBLE);

	cw_station3.begin(time + random(3000));
	cw_station3.set_station_state(AUDIBLE);

	cw_station4.begin(time + random(4000));
	cw_station4.set_station_state(AUDIBLE);

	cw_station5.begin(time + random(5000));
	cw_station5.set_station_state(AUDIBLE);

	cw_station6.begin(time + random(6000));
	cw_station6.set_station_state(AUDIBLE);

	cw_station7.begin(time + random(7000));
	cw_station7.set_station_state(AUDIBLE);

	cw_station8.begin(time + random(8000));
	cw_station8.set_station_state(AUDIBLE);

	cw_station9.begin(time + random(9000));
	cw_station9.set_station_state(AUDIBLE);

	cw_station10.begin(time + random(10000));
	cw_station10.set_station_state(AUDIBLE);

	// Initialize 5 Numbers stations
	numbers_station1.begin(time + random(11000));
	numbers_station1.set_station_state(AUDIBLE);

	numbers_station2.begin(time + random(12000));
	numbers_station2.set_station_state(AUDIBLE);

	numbers_station3.begin(time + random(13000));
	numbers_station3.set_station_state(AUDIBLE);

	numbers_station4.begin(time + random(14000));
	numbers_station4.set_station_state(AUDIBLE);

	numbers_station5.begin(time + random(15000));
	numbers_station5.set_station_state(AUDIBLE);

	// Initialize 4 RTTY stations
	rtty_station1.begin(time + random(16000));
	rtty_station1.set_station_state(AUDIBLE);

	rtty_station2.begin(time + random(17000));
	rtty_station2.set_station_state(AUDIBLE);

	rtty_station3.begin(time + random(18000));
	rtty_station3.set_station_state(AUDIBLE);

	rtty_station4.begin(time + random(19000));
	rtty_station4.set_station_state(AUDIBLE);

	// Initialize 2 Pager stations
	pager_station1.begin(time + random(20000));
	pager_station1.set_station_state(AUDIBLE);

	pager_station2.begin(time + random(21000));
	pager_station2.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FILE_PILE_UP
	// Initialize five CW pile-up stations (all calling BS77H)
	// Staggered start times to simulate realistic pile-up behavior
	// NOTE: Skip manual initialization when dynamic pipelining is enabled
	if (!station_manager.isDynamicPipeliningEnabled()) {
		cw_station1.begin(time + random(1000));
		cw_station1.set_station_state(AUDIBLE);
		
		cw_station2.begin(time + random(2000));
		cw_station2.set_station_state(AUDIBLE);
		
		cw_station3.begin(time + random(3000));
		cw_station3.set_station_state(AUDIBLE);
		
		// cw_station4.begin(time + random(4000));
		// cw_station4.set_station_state(AUDIBLE);

		// cw_station5.begin(time + random(5000));
		// cw_station5.set_station_state(AUDIBLE);
	}
	// When dynamic pipelining is enabled, stations are initialized by setupPipeline()
#endif

#ifdef CONFIG_FIVE_CW_RESOURCE_TEST
	// Initialize five CW test stations (resource contention test)
	// Only 4 wave generators available, so one station should be dormant at any given time
	// Give all stations similar startup delays to create fair competition for initial allocation
	cw_station1.begin(time + random(2000));
	cw_station1.set_station_state(AUDIBLE);
	
	cw_station2.begin(time + random(2000));
	cw_station2.set_station_state(AUDIBLE);
	
	cw_station3.begin(time + random(2000));
	cw_station3.set_station_state(AUDIBLE);
	
	// cw_station4.begin(time + random(2000));
	// cw_station4.set_station_state(AUDIBLE);
	
	// if(!cw_station5.begin(time + random(2000))) {  // Check if begin() succeeds
	//	// If begin() fails, put station in retry state so it will try again during runtime
	//	cw_station5.set_retry_state(time + 1000);  // Retry in 1 second
	// }
	// cw_station5.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_TEST_PERFORMANCE
	// Initialize test performance station
	test_station.begin(time + random(1000));
	test_station.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FOUR_NUMBERS
	// Initialize four Numbers test stations
	numbers_station1.begin(time + random(1000));
	numbers_station1.set_station_state(AUDIBLE);
	
	numbers_station2.begin(time + random(2000));
	numbers_station2.set_station_state(AUDIBLE);
	
	numbers_station3.begin(time + random(3000));
	numbers_station3.set_station_state(AUDIBLE);
	
	numbers_station4.begin(time + random(4000));
	numbers_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FOUR_PAGER
	// Initialize four Pager test stations
	pager_station1.begin(time + random(1000));
	pager_station1.set_station_state(AUDIBLE);
	
	pager_station2.begin(time + random(2000));
	pager_station2.set_station_state(AUDIBLE);
	
	pager_station3.begin(time + random(3000));
	pager_station3.set_station_state(AUDIBLE);
	
	pager_station4.begin(time + random(4000));
	pager_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FOUR_RTTY
	// Initialize four RTTY test stations
	rtty_station1.begin(time + random(1000));
	rtty_station1.set_station_state(AUDIBLE);
	
	rtty_station2.begin(time + random(2000));
	rtty_station2.set_station_state(AUDIBLE);
	
	rtty_station3.begin(time + random(3000));
	rtty_station3.set_station_state(AUDIBLE);
	
	rtty_station4.begin(time + random(4000));
	rtty_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_FOUR_JAMMER
	// Initialize four Jammer test stations with different frequencies
	jammer_station1.begin(time + random(1000), 7003000.0);
	jammer_station1.set_station_state(AUDIBLE);
	
	jammer_station2.begin(time + random(2000), 7004000.0);
	jammer_station2.set_station_state(AUDIBLE);
	
	jammer_station3.begin(time + random(3000), 7005000.0);
	jammer_station3.set_station_state(AUDIBLE);
	
	jammer_station4.begin(time + random(4000), 7006000.0);
	jammer_station4.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_MINIMAL_CW
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);
#endif

#ifdef CONFIG_DEV_LOW_RAM
	// Initialize development configuration stations
#ifdef ENABLE_MORSE_STATION
	cw_station1.begin(time + random(1000));
	cw_station1.set_station_state(AUDIBLE);
#endif
	
#ifdef ENABLE_NUMBERS_STATION
	numbers_station1.begin(time + random(1000));
	numbers_station1.set_station_state(AUDIBLE);
#endif

#ifdef ENABLE_TEST_STATION
	test_station.begin(time + random(1000));
	test_station.set_station_state(AUDIBLE);
#endif
#endif

#ifdef CONFIG_PAGER2_TEST
	// Initialize original SimPager test station to isolate issue
	pager_test.begin(time + random(1000));
	pager_test.set_station_state(AUDIBLE);
#endif
	set_application(APP_SIMRADIO, &display);

	while(true){
		unsigned long time = millis();
				// Update signal meter decay (capacitor-like discharge)
		signal_meter.update(time);
		
		// Update StationManager with current VFO frequency
		// Only update when in VFO mode (dispatcher1)
		if (dispatcher == &dispatcher1) {
			Mode* current_mode = dispatcher->get_current_mode();
			if (current_mode) {
				// We know this is a VFO since we're in dispatcher1
				// Use static_cast since we've verified the type through dispatcher check
				VFO* current_vfo = static_cast<VFO*>(current_mode);
				station_manager.updateStations(current_vfo->_frequency);
			}
		}

#ifdef CONFIG_TEST_PERFORMANCE
		// Test station runs automatically - just listen to the audio output
		// to determine loop performance and upper limits for station design
#endif
		// --- PANEL LOCK LED OVERRIDE ---
        int lock_brightness = signal_meter.get_panel_led_brightness();
        if (lock_brightness > 0) {
            int pwm = (lock_brightness * PANEL_LOCK_LED_FULL_BRIGHTNESS) / (255 * PANEL_LED_BRIGHTNESS_DIVISOR);
            analogWrite(WHITE_PANEL_LED, pwm); // White LED lock indicator
        } else {
            analogWrite(WHITE_PANEL_LED, 0);
        }        // Comment out the old animation:
		realization_pool.step(time);

		// NOTE: Station step() calls are handled automatically by realization_pool.step()
		// No need for manual step() calls - RealizationPool architecture handles this

		encoder_handlerA.step();
		encoder_handlerB.step();

		// Step non-blocking title display if active
		dispatcher->step_title_display(&display);

		// check for changing dispatchers
		bool pressed = encoder_handlerB.pressed();
		bool long_pressed = encoder_handlerB.long_pressed();
		if(pressed || long_pressed){
			if(pressed){
				// char *title;
				switch(current_dispatcher){
					case 1:
						// 
						dispatcher = set_application(APP_SETTINGS, &display); // Go to Settings
						// current_dispatcher = 2;
						// title = (FSTR("AudioOut"));
						break;
						
					case 2:
						// Clear flashlight mode when leaving settings
						signal_meter.clear_flashlight_mode();
						// 
						dispatcher = set_application(APP_SIMRADIO, &display); // &dispatcher1;
						// current_dispatcher = 1;
						// title = (FSTR("SimRadio"));
						break;
				}

				purge_events();
			}
		}
		// Always check encoder state to keep internal driver logic running
		bool encoderA_changed = encoder_handlerA.changed();
		bool encoderB_changed = encoder_handlerB.changed();
		
		// Process encoder events only when not showing title (to prevent missed events)
		if (!dispatcher->is_showing_title()) {
			if(encoderA_changed){
				#ifdef DEBUG_PIPELINING
				// Minimal tuning debug - only show frequency changes
				Mode* current_mode = dispatcher->get_current_mode();
				if (current_mode && dispatcher == &dispatcher1) {
					VFO* current_vfo = static_cast<VFO*>(current_mode);
					Serial.print("VFO: ");
					Serial.println(current_vfo->_frequency);
				}
				#endif
				
				dispatcher->dispatch_event(&display, ID_ENCODER_TUNING, encoder_handlerA.diff(), 0);
				dispatcher->update_display(&display);
				dispatcher->update_signal_meter(&signal_meter);
				
				// // Test: Add StationManager call in encoder A handling (where the problem occurred)
				// station_manager.updateStations(7000000);
				
				dispatcher->update_realization();
			}

			if(encoderB_changed){
				dispatcher->dispatch_event(&display, ID_ENCODER_MODES, encoder_handlerB.diff(), 0);
				purge_events();  // Clear any noise/overshoot after mode change
				
				// Note: No immediate update_display() call here - let show_title() finish first
				dispatcher->update_realization();
			}
		}
		// Note: If showing title, encoder changes are detected but ignored - 
		// this keeps the encoder driver state machine running properly

		pressed = encoder_handlerA.pressed();
		long_pressed = encoder_handlerA.long_pressed();
		if(pressed || long_pressed){
			dispatcher->dispatch_event(&display, ID_ENCODER_TUNING, pressed, long_pressed);
		}
	}
}
