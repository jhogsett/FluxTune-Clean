#ifndef __SIM_PAGER2_H__
#define __SIM_PAGER2_H__

// Step 6: DUAL GENERATOR MODE - The critical test (where it always fails)
//#define ENABLE_FIRST_GENERATOR  // Use first generator only (original behavior)
//#define ENABLE_SECOND_GENERATOR // Use second generator only (for testing)  
#define ENABLE_DUAL_GENERATOR   // Use both generators for dual-tone (target behavior)

#include "async_pager.h"
#include "sim_transmitter.h"

class SignalMeter; // Forward declaration

// DTMF frequency definitions (Hz offset from VFO)
// Row frequencies (low frequencies)
#define DTMF_ROW_1    697.0     // Rows 1, 2, 3
#define DTMF_ROW_2    770.0     // Rows 4, 5, 6  
#define DTMF_ROW_3    852.0     // Rows 7, 8, 9
#define DTMF_ROW_4    941.0     // Rows *, 0, #

// Column frequencies (high frequencies)
#define DTMF_COL_1    1209.0    // Columns 1, 4, 7, *
#define DTMF_COL_2    1336.0    // Columns 2, 5, 8, 0
#define DTMF_COL_3    1477.0    // Columns 3, 6, 9, #
#define DTMF_COL_4    1633.0    // Columns A, B, C, D (extended DTMF)

class SimPager2 : public SimTransmitter
{
public:
    SimPager2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq);
    
    virtual bool begin(unsigned long time) override;
    virtual bool update(Mode *mode) override;
    virtual bool step(unsigned long time) override;
    virtual void end() override;
    
    void realize();
    
    // Debug method to display current tone pair
    void debug_print_tone_pair() const;
    
    // Method to generate new tone pairs for testing
    void generate_new_tone_pair();
    
    // Method to generate DTMF digit tone pairs
    void generate_dtmf_digit();
    
    // Debug method to test dual generator acquisition
    void debug_test_dual_generator_acquisition();

private:
    AsyncPager _pager;
    float _current_tone_a_offset;
    float _current_tone_b_offset;
    SignalMeter *_signal_meter;     // Pointer to signal meter for charge pulses
    
    // DTMF digit tracking
    char _current_dtmf_digit_1;     // First generator's DTMF digit
    char _current_dtmf_digit_2;     // Second generator's DTMF digit
    
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    // Second generator support - separate wave generator for testing/dual-tone
    int _realizer_b;                // Second wave generator realizer ID
    float _current_tone_a_offset_b; // Second generator's tone A frequency offset
    float _current_tone_b_offset_b; // Second generator's tone B frequency offset
    
    // Helper methods for second generator management
    bool acquire_second_generator();
    void release_second_generator();
    void silence_second_generator();
#endif
};

#endif
