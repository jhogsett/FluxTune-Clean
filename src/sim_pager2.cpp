#include "vfo.h"
#include "wavegen.h"
#include "wave_gen_pool.h"
#include "sim_pager2.h"
#include "signal_meter.h"

SimPager2::SimPager2(WaveGenPool *wave_gen_pool, SignalMeter *signal_meter, float fixed_freq) 
    : SimTransmitter(wave_gen_pool, fixed_freq), _signal_meter(signal_meter)
{
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    _realizer_b = -1;
#endif

    // Generate initial tone pair
    generate_new_tone_pair();
    // Pager transmission will be started in begin() method
}

bool SimPager2::begin(unsigned long time)
{
#ifdef ENABLE_FIRST_GENERATOR
    if(!common_begin(time, _fixed_freq))
        return false;// Start pager transmission with repeat enabled
    _pager.start_pager_transmission(true);

    // Check if we have a valid realizer before accessing it
    if(_realizer == -1) {
        return false;  // No realizer available
    }

    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);

    // Initialize both channels to silent
    wavegen->set_frequency(SILENT_FREQ, false);
    wavegen->set_frequency(SILENT_FREQ, true);
#endif

#ifdef ENABLE_SECOND_GENERATOR
    // SECOND GENERATOR MODE: Use second generator infrastructure
    if(!acquire_second_generator()) {
        return false;  // No second generator available
    }
    
    // Start pager transmission with repeat enabled
    _pager.start_pager_transmission(true);

    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);

    // Initialize both channels to silent
    wavegen_b->set_frequency(SILENT_FREQ, false);
    wavegen_b->set_frequency(SILENT_FREQ, true);
#endif

    return true;
}

void SimPager2::realize()
{
    if(!check_frequency_bounds()) {
        return;  // Out of audible range
    }
    
#ifdef ENABLE_FIRST_GENERATOR
    // Don't try to access wave generator if we don't have one (during silence)
    if(_realizer == -1) {
        return;
    }
    
    WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
    
    if(_active) {
        // Set frequencies based on current pager state
        switch(_pager.get_current_state()) {
            case PAGER_STATE_TONE_A:
                // Transmit Tone A (longer unsquelch tone)
                wavegen->set_frequency(_frequency + _current_tone_a_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_a_offset, false);
                break;
                
            case PAGER_STATE_TONE_B:
                // Transmit Tone B (shorter identification tone)
                wavegen->set_frequency(_frequency + _current_tone_b_offset, true);
                wavegen->set_frequency(_frequency + _current_tone_b_offset, false);
                break;
                  default:
                // Silent state (SILENCE) - should not reach here when _active is true
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                break;
        }    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen->set_frequency(SILENT_FREQ, true);
        wavegen->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen->set_active_frequency(_active);
#endif

#ifdef ENABLE_SECOND_GENERATOR
    // SECOND GENERATOR MODE: Use second generator with its own tone offsets
    if(_realizer_b == -1) {
        return;
    }
    
    WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
    
    if(_active) {
        // Set frequencies based on current pager state - using second generator tone offsets
        switch(_pager.get_current_state()) {
            case PAGER_STATE_TONE_A:
                // Transmit Tone A (longer unsquelch tone) - second generator frequencies
                wavegen_b->set_frequency(_frequency + _current_tone_a_offset_b, true);
                wavegen_b->set_frequency(_frequency + _current_tone_a_offset_b, false);
                break;
                
            case PAGER_STATE_TONE_B:
                // Transmit Tone B (shorter identification tone) - second generator frequencies
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset_b, true);
                wavegen_b->set_frequency(_frequency + _current_tone_b_offset_b, false);
                break;
                  default:
                // Silent state (SILENCE) - should not reach here when _active is true
                wavegen_b->set_frequency(SILENT_FREQ, true);
                wavegen_b->set_frequency(SILENT_FREQ, false);
                break;
        }    } else {
        // Explicitly set silent frequencies when inactive (during SILENCE)
        wavegen_b->set_frequency(SILENT_FREQ, true);
        wavegen_b->set_frequency(SILENT_FREQ, false);
    }
    
    wavegen_b->set_active_frequency(_active);
#endif
}

bool SimPager2::update(Mode *mode)
{
    common_frequency_update(mode);

    if(_enabled) {
        // Note: We don't set frequencies here like RTTY does
        // Pager frequencies are set in realize() based on current tone
    }

    realize();
    return true;
}

bool SimPager2::step(unsigned long time)
{
    switch(_pager.step_pager(time)) {        case STEP_PAGER_TURN_ON:
            // Check if this is the start of a new page cycle (silence → tone A)
            if (_pager.get_current_state() == PAGER_STATE_TONE_A) {
                generate_new_tone_pair();
                
#ifdef ENABLE_FIRST_GENERATOR
                // RESOURCE MANAGEMENT: Acquire wave generator after silent period
                // Need to get a new realizer if we freed it during silence
                if(_realizer == -1) {
                    if(!common_begin(time, _fixed_freq)) {
                        // Failed to get wave generator - stay inactive
                        _active = false;
                        return true;
                    }
                    // CRITICAL: Force frequency update after reacquiring generator
                    force_frequency_update();
                }
#endif

#ifdef ENABLE_SECOND_GENERATOR
                // SECOND GENERATOR MODE: Acquire second generator after silent period
                if(_realizer_b == -1) {
                    if(!acquire_second_generator()) {
                        // Failed to get second generator - stay inactive
                        _active = false;
                        return true;
                    }
                    // CRITICAL: Force frequency update after reacquiring generator
                    force_frequency_update();
                }
#endif
            }
            _active = true;
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse when carrier turns on
            break;

        case STEP_PAGER_LEAVE_ON:
            // Carrier remains on - send another charge pulse
            send_carrier_charge_pulse(_signal_meter);
            break;

        case STEP_PAGER_TURN_OFF:
            _active = false;
            realize();
            
#ifdef ENABLE_FIRST_GENERATOR
            // RESOURCE MANAGEMENT: Release wave generator during silent period
            // First ensure frequencies are silenced
            if(_realizer != -1) {
                WaveGen *wavegen = _wave_gen_pool->access_realizer(_realizer);
                wavegen->set_frequency(SILENT_FREQ, true);
                wavegen->set_frequency(SILENT_FREQ, false);
                wavegen->set_active_frequency(false);
            }
            // Release the resource for other stations to use during silence
            end();  // This calls Realization::end() which frees the realizer
#endif

#ifdef ENABLE_SECOND_GENERATOR
            // SECOND GENERATOR MODE: Release second generator during silent period
            silence_second_generator();
            release_second_generator();
#endif
            
            // No charge pulse when carrier turns off
            break;
              case STEP_PAGER_CHANGE_FREQ:
            // Transmitter stays on, but frequency needs to change
            realize();
            send_carrier_charge_pulse(_signal_meter);  // Send charge pulse on frequency change while on
            break;
            
        // LEAVE_ON and LEAVE_OFF don't require action since _active state doesn't change
        // and no frequency update is needed during silence
    }

    return true;
}

void SimPager2::generate_new_tone_pair()
{
    // Generate random tone pair similar to DTMF frequencies
    // Range: 650-1650 Hz offset, minimum 200 Hz separation
    
    float frequency_range = PAGER2_TONE_MAX_OFFSET - PAGER2_TONE_MIN_OFFSET;
    
    // Arduino random() function
    _current_tone_a_offset = PAGER2_TONE_MIN_OFFSET + 
        random((long)(frequency_range - PAGER2_TONE_MIN_SEPARATION));
    
    // Generate second tone with minimum separation
    float remaining_range = frequency_range - PAGER2_TONE_MIN_SEPARATION;
    float tone_b_base = random((long)remaining_range);
    
    // Ensure minimum separation
    if (tone_b_base < _current_tone_a_offset - PAGER2_TONE_MIN_OFFSET) {
        _current_tone_b_offset = PAGER2_TONE_MIN_OFFSET + tone_b_base;
    } else {
        _current_tone_b_offset = _current_tone_a_offset + PAGER2_TONE_MIN_SEPARATION + 
            (tone_b_base - (_current_tone_a_offset - PAGER2_TONE_MIN_OFFSET));
    }

    // Ensure tone B doesn't exceed maximum
    if (_current_tone_b_offset > PAGER2_TONE_MAX_OFFSET) {
        _current_tone_b_offset = PAGER2_TONE_MAX_OFFSET;
    }

#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
    // Second generator gets its own completely separate tone pair (IMPORTANT: different frequencies)
    _current_tone_a_offset_b = PAGER2_TONE_MIN_OFFSET + 
        random((long)(frequency_range - PAGER2_TONE_MIN_SEPARATION));
    
    // Generate second tone for generator B with minimum separation
    float tone_b_base_b = random((long)remaining_range);
    
    // Ensure minimum separation for generator B
    if (tone_b_base_b < _current_tone_a_offset_b - PAGER2_TONE_MIN_OFFSET) {
        _current_tone_b_offset_b = PAGER2_TONE_MIN_OFFSET + tone_b_base_b;
    } else {
        _current_tone_b_offset_b = _current_tone_a_offset_b + PAGER2_TONE_MIN_SEPARATION + 
            (tone_b_base_b - (_current_tone_a_offset_b - PAGER2_TONE_MIN_OFFSET));
    }

    // Ensure tone B for generator B doesn't exceed maximum
    if (_current_tone_b_offset_b > PAGER2_TONE_MAX_OFFSET) {
        _current_tone_b_offset_b = PAGER2_TONE_MAX_OFFSET;
    }
#endif
}

void SimPager2::debug_print_tone_pair() const
{
    // Debug output not needed for Arduino build
}

void SimPager2::end()
{
    // Call parent class end method
    SimTransmitter::end();
}

// SECOND GENERATOR HELPER METHODS
#if defined(ENABLE_SECOND_GENERATOR) || defined(ENABLE_DUAL_GENERATOR)
bool SimPager2::acquire_second_generator()
{
    if (_realizer_b != -1) {
        return true;  // Already have one
    }
    
    _realizer_b = _wave_gen_pool->get_realizer(_station_id);
    return (_realizer_b != -1);
}

void SimPager2::release_second_generator()
{
    if (_realizer_b != -1) {
        _wave_gen_pool->free_realizer(_realizer_b, _station_id);
        _realizer_b = -1;
    }
}

void SimPager2::silence_second_generator()
{
    if (_realizer_b != -1) {
        WaveGen *wavegen_b = _wave_gen_pool->access_realizer(_realizer_b);
        if (wavegen_b) {
            wavegen_b->set_frequency(SILENT_FREQ, true);
            wavegen_b->set_frequency(SILENT_FREQ, false);
            wavegen_b->set_active_frequency(false);
        }
    }
}
#endif
