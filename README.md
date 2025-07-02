# FluxTune
Arduino Based Artificial Radio with Dynamic Station Pipelining

![20250624_144336(1)](https://github.com/user-attachments/assets/a3883e58-d091-49f9-a104-2a4143bb5c5b)

## Features

FluxTune is a sophisticated radio simulation system that creates an "infinite band" experience using a dynamic pipelining system:

### 🎯 Dynamic Station Pipelining
- **Infinite Band Experience**: Stations appear dynamically as you tune across the band
- **Resource Efficient**: Fixed pool of 3-5 stations recycled intelligently  
- **Non-Intrusive**: Doesn't interrupt audible transmissions
- **Responsive**: Stations move ahead/behind VFO based on tuning direction
- **Memory Safe**: No dynamic allocation, works within Arduino constraints

### 🚀 Station Types
- **CW/Morse Stations**: Customizable callsigns, speeds, and "fist quality" (timing variation)
- **Numbers Stations**: Authentic spooky number group transmissions  
- **Pager Stations**: Digital pager simulations
- **RTTY Stations**: Radio teletype digital mode
- **Jammer Stations**: Interference testing and simulation

### 📊 Signal Meter
- 7-LED WS2812 signal strength indicator
- Real-time response to frequency proximity
- Updates synchronously with VFO tuning
- Visual feedback for station activity

### ⚙️ Configurable Architecture
- Conditional compilation for different station mixes
- Optimized for Arduino Nano (2KB RAM, 30KB Flash)
- Event-driven architecture with clean separation
- Multiple VFO support (VFO A/B/C)

## Configuration

### Station Configuration
Edit `include/station_config.h` to choose your configuration:

**Production (Default):**
```cpp
#define CONFIG_FILE_PILE_UP     // 3 CW stations for pipelining (recommended)
```

**Alternative Configurations:**
```cpp
#define CONFIG_MIXED_STATIONS   // All different station types
#define CONFIG_FOUR_CW          // Four CW stations (different speeds)  
#define CONFIG_FIVE_CW          // Five CW stations (Field Day simulation)
#define CONFIG_CW_CLUSTER       // CW stations clustered for listening pleasure
#define CONFIG_FOUR_NUMBERS     // Four Numbers stations (spooky!)
#define CONFIG_FOUR_PAGER       // Four Pager stations
#define CONFIG_FOUR_RTTY        // Four RTTY stations
#define CONFIG_MINIMAL_CW       // Single CW station (minimal memory)
```

### Pipelining Configuration
Adjust pipelining parameters in `include/station_manager.h`:

```cpp
#define PIPELINE_LOOKAHEAD_RANGE 5000    // 5 kHz ahead/behind VFO
#define PIPELINE_AUDIBLE_RANGE 5000      // Range where stations become audible  
#define PIPELINE_REALLOC_THRESHOLD 3000  // Reallocate when VFO moves 3 kHz
```

## Building and Deployment

This project uses PlatformIO for Arduino development:

```bash
# Build firmware
platformio run

# Upload to Arduino Nano
platformio run --target upload

# Monitor serial output  
platformio device monitor
```

### Hardware Requirements
- **Arduino Nano** (ATmega328P)
- **4x AD9833 DDS modules** for audio generation
- **7x WS2812 LEDs** for signal meter
- **2x Rotary encoders** for VFO tuning and mode selection
- **HT16K33 display** for frequency readout

## How Dynamic Pipelining Works

FluxTune creates the illusion of an infinite band using a clever recycling system:

1. **Station Pool**: A fixed number of station objects (typically 3-5)
2. **Frequency Tracking**: VFO frequency changes are monitored continuously  
3. **Smart Recycling**: When you tune away from a station, it gets recycled to appear ahead of your tuning direction
4. **State Awareness**: Only dormant or distant stations are moved to avoid cutting off audible transmissions
5. **Natural Behavior**: Stations maintain their characteristic timing, drift, and transmission patterns

### Example Scenario
- Start at 7.000 MHz with stations at 7.001, 7.002, 7.003 MHz
- Tune up to 7.005 MHz  
- Station at 7.001 MHz (now 4 kHz behind) gets recycled to 7.008 MHz (3 kHz ahead)
- Continue tuning and more stations appear dynamically ahead of your direction

This creates a realistic "band activity" experience where stations are always present as you explore the frequency spectrum.

## Architecture

FluxTune uses a clean event-driven architecture optimized for Arduino:

### Core Components
- **StationManager**: Handles dynamic pipelining and station lifecycle
- **Event Dispatcher**: Central event routing for VFO tuning and UI
- **Mode Handlers**: VFO tuning, contrast control, BFO offset, flashlight
- **Display System**: HT16K33-based frequency display with scrolling
- **Signal Meter**: WS2812 LED strip with proximity-based intensity
- **Station Pool**: Configurable station types with conditional compilation

### Memory Management
- **Static Allocation**: No dynamic memory allocation for Arduino stability
- **Conditional Compilation**: Only compile station types you need
- **Optimized Footprint**: Fits comfortably in Arduino Nano constraints
- **Resource Pooling**: Shared AD9833 channels among active stations

### Station State Machine
Each station follows a lifecycle:
1. **DORMANT**: Not transmitting, available for recycling
2. **ACTIVE**: Transmitting but outside audible range
3. **AUDIBLE**: Transmitting within hearing range (protected from interruption)
4. **SILENT**: Between transmissions but maintaining frequency

The pipelining system respects these states to create realistic radio behavior without jarring interruptions.

## Development

### Branch Structure
- **main**: Stable release code
- **feature/dynamic-pipelining**: Latest pipelining improvements
- Development follows feature branch workflow

### Code Quality
- Modern C++ practices within Arduino constraints
- Comprehensive inline documentation
- GitHub Copilot reviewed for best practices
- Memory-efficient design patterns

## License

This project demonstrates advanced Arduino programming techniques for audio synthesis, real-time signal processing, and efficient resource management in constrained environments.
