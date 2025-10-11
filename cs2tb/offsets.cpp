#include "offsets.hpp"
#include <string>
#include <sstream>
#include <winhttp.h>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

uintptr_t dwEntityList = 0x0;
uintptr_t dwLocalPlayerPawn = 0x0;
uintptr_t dwViewMatrix = 0x0;
uintptr_t m_iHealth = 0x0;
uintptr_t m_iTeamNum = 0x0;
uintptr_t m_iIDEntIndex = 0x0;
uintptr_t m_vOldOrigin = 0x0;

std::string DownloadTextFile(const std::wstring& url)
{
    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256]{};
    wchar_t urlPath[1024]{};

    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = _countof(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = _countof(urlPath);

    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp))
        return {};

    HINTERNET hSession = WinHttpOpen(L"lolikuza",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    std::string result;
    DWORD bytesRead = 0;
    char buffer[4096]{};

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        result.append(buffer, bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

uintptr_t ParseOffset(const std::string& line)
{
    size_t pos = line.find("0x");
    if (pos == std::string::npos) return 0;
    return std::stoul(line.substr(pos), nullptr, 16);
}

void LoadOffsetsFromText(const std::string& text, bool isClientDll)
{
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        if (!isClientDll)
        {
            if (line.find("dwEntityList") != std::string::npos)
                dwEntityList = ParseOffset(line);
            else if (line.find("dwLocalPlayerPawn") != std::string::npos)
                dwLocalPlayerPawn = ParseOffset(line);
        }
        else
        {
            if (line.find("m_iHealth") != std::string::npos)
                m_iHealth = ParseOffset(line);
            else if (line.find("m_iTeamNum") != std::string::npos)
                m_iTeamNum = ParseOffset(line);
            else if (line.find("m_iIDEntIndex") != std::string::npos)
                m_iIDEntIndex = ParseOffset(line);
        }
    }
}

void UpdateOffsets()
{
    std::wstring urlOffsets = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.hpp";
    std::wstring urlClient = L"https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp";

    std::string offsetsText = DownloadTextFile(urlOffsets);
    if (!offsetsText.empty())
    {
        LoadOffsetsFromText(offsetsText, false);
    }

    std::string clientText = DownloadTextFile(urlClient);
    if (!clientText.empty())
    {
        LoadOffsetsFromText(clientText, true);
    }
}
