#ifndef STATION_MANAGER_H
#define STATION_MANAGER_H

#include "sim_transmitter.h"
#include <stdint.h>

#define MAX_STATIONS 6
#define MAX_AD9833 4

class StationManager {
public:
    StationManager(SimTransmitter* station_ptrs[MAX_STATIONS]);
    void updateStations(uint32_t vfo_freq);
    void allocateAD9833();
    void recycleDormantStations(uint32_t vfo_freq);
    SimTransmitter* getStation(int idx);
    int getActiveStationCount() const;

private:
    SimTransmitter* stations[MAX_STATIONS];
    int ad9833_assignment[MAX_AD9833]; // Maps AD9833 channels to station indices
    void activateStation(int idx, uint32_t freq);
    void deactivateStation(int idx);
    int findDormantStation();
};

#endif // STATION_MANAGER_H
