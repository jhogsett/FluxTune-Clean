# FluxTune
Arduino Based Artificial Radio

![20250624_144336(1)](https://github.com/user-attachments/assets/a3883e58-d091-49f9-a104-2a4143bb5c5b)

## Features

FluxTune is a sophisticated radio simulation system with multiple station types and a frequency-aware signal meter:

### 🚀 Station Types
- **CW/Morse Stations**: Customizable callsigns and speeds
- **Numbers Stations**: Authentic spooky number group transmissions  
- **Pager Stations**: Digital pager simulations
- **RTTY Stations**: Radio teletype digital mode

### 📊 Signal Meter
- 7-LED WS2812 signal strength indicator
- Real-time response to frequency proximity
- Updates synchronously with VFO tuning

### ⚙️ Configurable Architecture
- Conditional compilation for memory optimization
- Switchable test configurations
- Event-driven architecture with clean separation

## Configuration

### Station Configuration
Edit `include/station_config.h` to choose your configuration:

**Production (Default):**
```cpp
#define CONFIG_MIXED_STATIONS    // All different station types
```

**Testing Configurations:**
```cpp
#define CONFIG_FOUR_CW          // Four CW stations (different speeds)
#define CONFIG_FOUR_NUMBERS     // Four Numbers stations (spooky!)
#define CONFIG_FOUR_PAGER       // Four Pager stations
#define CONFIG_FOUR_RTTY        // Four RTTY stations  
#define CONFIG_MINIMAL_CW       // Single CW station (minimal memory)
```

See [STATION_CONFIGURATION.md](STATION_CONFIGURATION.md) for detailed configuration options.

## Testing

This project includes comprehensive test suites for core components:

### AsyncMorse Tests
- 23 tests covering Morse code generation, timing, and behavior
- Tests initialization, dot/dash patterns, timing consistency, and edge cases

### AsyncRTTY Tests  
- 20 tests covering RTTY signal generation, timing, and behavior
- Tests initialization, state transitions, bit timing, repeat modes, and randomness

### Running Tests

The tests use the Unity test framework and can be run natively on PC:

```bash
# AsyncMorse tests
g++ -DNATIVE_BUILD -I. -Itest/unity/src -std=c++11 test/test_async_morse.cpp test/unity/src/unity.c src/async_morse.cpp src/utils.cpp src/buffers.cpp src/saved_data.cpp native/mock_arduino.cpp native/mock_wire.cpp native/mock_eeprom.cpp native/mock_ht16k33disp.cpp -o test_async_morse && ./test_async_morse

# AsyncRTTY tests
g++ -DNATIVE_BUILD -I. -Itest/unity/src -DUNITY_INCLUDE_DOUBLE test/test_async_rtty.cpp test/unity/src/unity.c src/async_rtty.cpp native/mock_arduino.cpp -o test_async_rtty && ./test_async_rtty
```

**Note for PowerShell users:** Replace `&&` with `;` for command chaining:
```powershell
# Example for PowerShell
g++ ... -o test_async_morse; ./test_async_morse
```

VS Code tasks are also available for building and running tests.

## Architecture

FluxTune uses a clean event-driven architecture:
- **Event Dispatcher**: Central event routing and coordination
- **Mode Handlers**: VFO tuning, contrast control, option management
- **Display System**: Synchronized updates with signal meter
- **Station Pool**: Configurable realizations with conditional compilation

The architecture supports both native PC testing and Arduino deployment with the same codebase.
