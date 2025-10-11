#include "tb.hpp"
#include "memory.hpp"
#include "offsets.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

bool TriggerBot::initialize() {
    while (true) {
        hSerial = connectArduino(COM_PORT);
        if (hSerial != INVALID_HANDLE_VALUE)
            break;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return true;
}

HANDLE TriggerBot::connectArduino(const char* port) {
    HANDLE hSerial = CreateFileA(
        port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_ACCESS_DENIED) {
            std::cout << "device not found, retrying...\n";
        }
        else {
            std::cout << "error opening device (" << err << "), retrying...\n";
        }
        return INVALID_HANDLE_VALUE;
    }

    DCB params = { 0 };
    params.DCBlength = sizeof(params);
    if (!GetCommState(hSerial, &params)) {
        std::cout << "device not found, retrying...\n";
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    params.BaudRate = BAUD_RATE;
    params.ByteSize = 8;
    params.StopBits = ONESTOPBIT;
    params.Parity = NOPARITY;

    if (!SetCommState(hSerial, &params)) {
        std::cout << "device not found, retrying...\n";
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutConstant = 50;
    SetCommTimeouts(hSerial, &timeouts);

    std::cout << "connected to the device. " << port << "\n";
    return hSerial;
}

bool TriggerBot::sendMouseClick() {
    if (hSerial == INVALID_HANDLE_VALUE) {
        HANDLE newSerial = connectArduino(COM_PORT);
        if (newSerial != INVALID_HANDLE_VALUE) {
            hSerial = newSerial;
            if (!isArduinoConnected) {
                std::cout << "connected to the device. " << COM_PORT << "\n";
                isArduinoConnected = true;
            }
        }
        else {
            if (isArduinoConnected) {
                std::cerr << "device not found, retrying...\n";
                isArduinoConnected = false;
            }
        }
        return false;
    }

    DWORD bytesWritten = 0;
    const char* msg = "loli\n";
    if (!WriteFile(hSerial, msg, static_cast<DWORD>(strlen(msg)), &bytesWritten, nullptr)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        if (isArduinoConnected) {
            std::cerr << "device not found, retrying...\n";
            isArduinoConnected = false;
        }
        return false;
    }

    return true;
}

bool TriggerBot::isAltPressed() {
    return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
}

bool TriggerBot::isWindowActive(const std::string& namePart) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;

    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    return std::string(title).find(namePart) != std::string::npos;
}

void TriggerBot::run() {
    c_game game;
    if (!game.initialize()) {
        std::cerr << "[!] cs2.exe\n";
        return;
    }

    auto clientBase = game.get_client_base();

    while (true) {
        if (!isWindowActive("Counter-Strike 2")) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (hSerial == INVALID_HANDLE_VALUE) {
            hSerial = connectArduino(COM_PORT);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (isAltPressed()) {
            auto localPlayer = game.read<uintptr_t>(clientBase + dwLocalPlayerPawn);
            if (!localPlayer.has_value() || localPlayer.value() == 0)
                continue;

            int entityId = game.read<int>(localPlayer.value() + m_iIDEntIndex).value_or(0);
            if (entityId > 0) {
                uintptr_t entityList = game.read<uintptr_t>(clientBase + dwEntityList).value_or(0);
                uintptr_t entry = game.read<uintptr_t>(entityList + 0x8 * (entityId >> 9) + 0x10).value_or(0);
                uintptr_t entity = game.read<uintptr_t>(entry + 120 * (entityId & 0x1FF)).value_or(0);

                if (entity) {
                    int targetHp = game.read<int>(entity + m_iHealth).value_or(0);
                    int targetTeam = game.read<int>(entity + m_iTeamNum).value_or(0);
                    int myTeam = game.read<int>(localPlayer.value() + m_iTeamNum).value_or(0);

                    if (targetHp > 0 && targetTeam != myTeam) {
                        static std::random_device rd;
                        static std::mt19937 gen(rd());
                        static std::uniform_int_distribution<> dis(0, 20);
                        int delay = dis(gen);

                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        sendMouseClick();
                        std::this_thread::sleep_for(std::chrono::milliseconds(45));
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
