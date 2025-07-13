# SimQSO Design Specification
*QSO Station Simulation for FluxTune*

## **Design Philosophy**

Your approach is **much more elegant** than a composite station design. Instead of managing two separate station objects, create a single `SimQSO` station that **internally simulates both sides** of a conversation.

## **Core Architecture**

```cpp
class SimQSO : public SimTransmitter {
private:
    // Station A characteristics (first operator)
    char callsign_a[8];     // e.g., "W1ABC"
    uint8_t wpm_a;          // e.g., 18 WPM
    uint8_t fist_a;         // e.g., 30 (slight timing variation)
    
    // Station B characteristics (second operator) 
    char callsign_b[8];     // e.g., "VK2DEF"
    uint8_t wpm_b;          // e.g., 22 WPM  
    uint8_t fist_b;         // e.g., 10 (precise timing)
    
    // QSO-wide characteristics
    float base_frequency;   // e.g., 7.025000 MHz (shared frequency)
    float freq_offset_b;    // e.g., +50 Hz when station B transmits
    
    // QSO state management
    QSOState current_state; // Which part of QSO script we're in
    bool transmitting_a;    // true = A transmitting, false = B transmitting
    unsigned long phase_start_time; // When current phase began
    
    // QSO script progression
    uint8_t script_index;   // Current position in QSO sequence
    
public:
    SimQSO(WaveGenPool* pool, SignalMeter* meter, float freq);
    virtual bool step(unsigned long time) override;
    virtual void randomize() override; // Generate new callsigns/characteristics
};
```

## **QSO Script Phases**

```cpp
enum QSOState {
    QSO_CQ_ROUND_1,     // Station A calling CQ (first attempt)
    QSO_CQ_ROUND_2,     // Station A calling CQ (second attempt)  
    QSO_CQ_ROUND_3,     // Station A calling CQ (third attempt - optional)
    QSO_LISTENING_A,    // Station A listening after CQ
    QSO_ANSWERING,      // Station B finally responds
    QSO_RST_EXCHANGE,   // Exchange signal reports
    QSO_QTH_EXCHANGE,   // Exchange locations
    QSO_CHAT,           // Brief conversation (optional)
    QSO_73s,            // Final 73s
    QSO_COMPLETE,       // QSO finished, pause before next
    QSO_IDLE            // Between QSOs
};
```

## **Realistic Audio Behavior**

### **Frequency Management:**
```cpp
void SimQSO::updateFrequency() {
    if (transmitting_a) {
        // Station A transmits on exact base frequency
        setTransmitFrequency(base_frequency);
    } else {
        // Station B slightly off frequency (realistic detuning)
        setTransmitFrequency(base_frequency + freq_offset_b);
    }
}
```

### **Timing Characteristics:**
- Each operator has their own WPM and fist quality
- Realistic pauses between transmissions
- Station A might be faster/slower than Station B
- Natural QSO rhythm with brief dead air

## **Sample QSO Script**

```cpp
struct QSOPhase {
    QSOState state;
    bool station_a_transmits;
    const char* message_template;
    uint16_t duration_ms;
    uint16_t listen_time_ms;  // Silent time after transmission
};

const QSOPhase standard_qso[] = {
    // Station A calls CQ multiple times before getting response
    {QSO_CQ_ROUND_1,  true,  "CQ CQ CQ DE %s %s K", 6000, 3000},    // CQ + 3 sec listen
    {QSO_CQ_ROUND_2,  true,  "CQ CQ DE %s %s K", 5000, 4000},       // Shorter CQ + 4 sec listen  
    {QSO_CQ_ROUND_3,  true,  "CQ CQ DE %s %s PSE K", 5000, 5000},   // Final CQ + 5 sec listen
    
    // Station B finally responds (simulates discovering the CQ)
    {QSO_ANSWERING,   false, "%s DE %s %s KN", 4000, 1000},
    
    // Normal exchange continues
    {QSO_RST_EXCHANGE, true,  "%s DE %s UR 5NN 5NN QTH %s %s K", 6000, 1000},
    {QSO_RST_EXCHANGE, false, "%s DE %s R R TNX 599 599 QTH %s %s K", 6000, 1000},
    {QSO_73s,         true,  "%s DE %s 73 73 %s SK", 4000, 1000},
    {QSO_73s,         false, "%s DE %s 73 GL %s SK", 3000, 0},
    {QSO_COMPLETE,    false, "", 0, 15000}, // 15 second pause before next QSO
    {QSO_IDLE,        false, "", 0, 0}      // End marker
};
```

### **Realistic CQ Discovery Behavior:**

**Station A (CQ-ing station):**
- Sends 2-3 CQ rounds with slight variations
- Each CQ gets progressively shorter 
- Listens after each CQ for 3-5 seconds
- Might add "PSE" (please) on final CQ attempt

**Station B (Answering station):**
- Doesn't respond to first CQ (still tuning/deciding)
- May respond to second or third CQ
- Response timing randomized (30-70% chance per CQ round)
- Simulates operator discovering and deciding to answer

**Implementation Logic:**
```cpp
bool SimQSO::shouldStationBAnswer(QSOState cq_round) {
    static uint8_t response_threshold = random(40, 80); // 40-80% threshold
    uint8_t current_chance;
    
    switch(cq_round) {
        case QSO_CQ_ROUND_1: current_chance = 20; break;  // 20% chance
        case QSO_CQ_ROUND_2: current_chance = 50; break;  // 50% chance  
        case QSO_CQ_ROUND_3: current_chance = 85; break;  // 85% chance
        default: current_chance = 0;
    }
    
    return (current_chance >= response_threshold);
}
```

## **Key Advantages of This Approach**

### **1. Single Station Object**
- ✅ Fits perfectly into existing StationManager
- ✅ Uses only 1 AD9833 wave generator 
- ✅ Standard frequency allocation and pipelining
- ✅ No complex inter-station coordination needed

### **2. Realistic Audio Experience**
- ✅ Slight frequency offset creates authentic "two station" sound
- ✅ Different WPM speeds for each operator
- ✅ Different fist qualities create personality
- ✅ Natural QSO timing and flow

### **3. Memory Efficient**
- ✅ Single object vs. two separate stations
- ✅ Shared frequency management
- ✅ Compact script-based progression
- ✅ Minimal RAM overhead

### **4. Easy Integration**
```cpp
// Add to any configuration like any other station:
SimQSO qso_station(&wave_gen_pool, &signal_meter, 7025000.0);

Realization *realizations[4] = {
    &cw_station1,
    &qso_station,     // ← Just another station!
    &numbers_station,
    &rtty_station
};
```

## **Implementation Details**

### **Frequency Detuning:**
- **Station A**: Exactly on frequency (e.g., 7.025000 MHz)
- **Station B**: +30 to +80 Hz offset (randomized per QSO)
- **Effect**: Sounds like two different stations when listening

### **Timing Variation:**
- **CQ Discovery**: Station A calls 2-3 times before Station B responds
- **Listening periods**: 3-5 seconds of silence after each CQ
- **Response probability**: Increases with each CQ round (20% → 50% → 85%)
- **Between operators**: 500-2000ms pause when switching to exchange
- **Within message**: Each operator uses their own WPM/fist
- **Between QSOs**: 10-30 second random pause

### **Realistic Operating Patterns:**
```cpp
// Example timing for complete QSO cycle:
// 00:00 - Station A: "CQ CQ CQ DE W1ABC W1ABC K" (6 sec)
// 00:06 - Listen period (3 sec silence) 
// 00:09 - Station A: "CQ CQ DE W1ABC W1ABC K" (5 sec)
// 00:14 - Listen period (4 sec silence)
// 00:18 - Station B: "W1ABC DE VK2DEF VK2DEF KN" (4 sec)
// 00:22 - Normal exchange continues...
```

### **Content Variation:**
```cpp
void SimQSO::randomize() {
    // Generate new callsigns
    generateCallsign(callsign_a, NORTH_AMERICA);
    generateCallsign(callsign_b, OCEANIA);
    
    // Randomize characteristics  
    wpm_a = random(12, 25);
    wpm_b = random(15, 30);
    fist_a = random(10, 60);
    fist_b = random(5, 40);
    
    // Small frequency offset for station B
    freq_offset_b = random(30, 80);
}
```

## **Advanced Features (Future)**

### **QSO Types:**
- **Casual Ragchew**: Longer conversation
- **Contest Exchange**: Quick 599/state exchange  
- **DX Contact**: Pile-up with rare station
- **Emergency**: Formal emergency traffic

### **Propagation Effects:**
- **QSB**: Signal strength variation during QSO
- **QRM**: Interference from other stations
- **Static Crashes**: Atmospheric noise

### **Learning Mode:**
- **Procedural Training**: Proper QSO format
- **Callsign Recognition**: Practice copying calls
- **Speed Training**: Gradually increasing WPM

## **Memory Footprint Estimate**

```
Callsigns (2 × 8 bytes):     16 bytes
Characteristics:              4 bytes  
Frequency data:               8 bytes
State management:             8 bytes
Script tracking:              4 bytes
                             --------
Total additional:           ~40 bytes per SimQSO
```

**Conclusion**: Your design is **brilliant** - it provides realistic QSO simulation with minimal complexity and maximum compatibility with the existing architecture!
