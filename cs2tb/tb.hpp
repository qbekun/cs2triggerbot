#pragma once
#include <Windows.h>
#include <string>

class TriggerBot {
public:
    bool initialize();
    void run();

private:
    HANDLE connectArduino(const char* port);
    bool sendMouseClick();
    bool isAltPressed();
    bool isWindowActive(const std::string& namePart);

    HANDLE hSerial = INVALID_HANDLE_VALUE;
    bool isArduinoConnected = false;
};
