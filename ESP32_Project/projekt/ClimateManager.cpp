#include "ClimateManager.h"

ClimateManager::ClimateManager() : peltierState(false), fanState(false) {}

void ClimateManager::init() {
    // Deliberately empty - no pin activity
}

void ClimateManager::setPeltier(bool state) {
    peltierState = state;
}

void ClimateManager::setFan(bool state) {
    fanState = state;
}

void ClimateManager::togglePeltier() {
    setPeltier(!peltierState);
}

void ClimateManager::toggleFan() {
    setFan(!fanState);
}

bool ClimateManager::getPeltierState() const {
    return peltierState;
}

bool ClimateManager::getFanState() const {
    return fanState;
}

void ClimateManager::processCommand(const String& target, const String& stateCmd) {
    bool targetState = (stateCmd == "ON" || stateCmd == "1" || stateCmd == "true");
    (void)target;
    (void)targetState;
    // Deliberately empty - no pin or serial activity
}