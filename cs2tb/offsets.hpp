#pragma once
#include <Windows.h>

constexpr auto COM_PORT = "COM3";
constexpr DWORD BAUD_RATE = CBR_115200;

extern uintptr_t dwEntityList;
extern uintptr_t dwLocalPlayerPawn;
extern uintptr_t m_iHealth;
extern uintptr_t m_iTeamNum;
extern uintptr_t m_iIDEntIndex;

void UpdateOffsets();
