#include "station_manager.h"
#include "sim_numbers.h" // Example concrete station type

StationManager::StationManager(SimTransmitter* station_ptrs[MAX_STATIONS]) {
    for (int i = 0; i < MAX_STATIONS; ++i) {
        stations[i] = station_ptrs[i];
        stations[i]->setActive(false); // Set all stations dormant/inactive
    }
    for (int i = 0; i < MAX_AD9833; ++i) {
        ad9833_assignment[i] = -1;
    }
}

void StationManager::updateStations(uint32_t vfo_freq) {
    // TODO: Implement logic to activate/deactivate stations based on VFO frequency
}

void StationManager::allocateAD9833() {
    // TODO: Assign AD9833 channels to the most relevant stations
}

void StationManager::recycleDormantStations(uint32_t vfo_freq) {
    // TODO: Reuse dormant stations for new frequencies as needed
}

SimTransmitter* StationManager::getStation(int idx) {
    if (idx >= 0 && idx < MAX_STATIONS) return stations[idx];
    return nullptr;
}

int StationManager::getActiveStationCount() const {
    int count = 0;
    for (int i = 0; i < MAX_STATIONS; ++i) {
        if (stations[i]->isActive()) ++count;
    }
    return count;
}

void StationManager::activateStation(int idx, uint32_t freq) {
    // TODO: Set up station[idx] for the given frequency and mark as active
    if (idx >= 0 && idx < MAX_STATIONS) stations[idx]->setActive(true);
}

void StationManager::deactivateStation(int idx) {
    // TODO: Mark station[idx] as dormant and release any AD9833 assignment
    if (idx >= 0 && idx < MAX_STATIONS) stations[idx]->setActive(false);
}

int StationManager::findDormantStation() {
    for (int i = 0; i < MAX_STATIONS; ++i) {
        if (!stations[i]->isActive()) return i;
    }
    return -1;
}
