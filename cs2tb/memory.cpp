#include "memory.hpp"

static std::optional<DWORD> find_process_id(const std::wstring& name) {
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return std::nullopt;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (name == entry.szExeFile) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return std::nullopt;
}

static std::optional<std::uintptr_t> get_module_base(DWORD pid, const std::wstring& mod_name) {
    MODULEENTRY32W modEntry{};
    modEntry.dwSize = sizeof(modEntry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return std::nullopt;

    if (Module32FirstW(snapshot, &modEntry)) {
        do {
            if (mod_name == modEntry.szModule) {
                CloseHandle(snapshot);
                return reinterpret_cast<std::uintptr_t>(modEntry.modBaseAddr);
            }
        } while (Module32NextW(snapshot, &modEntry));
    }

    CloseHandle(snapshot);
    return std::nullopt;
}

bool c_game::initialize() {
    auto pid = find_process_id(L"cs2.exe");
    if (!pid.has_value())
        return false;

    m_process_id = pid.value();

    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, m_process_id.value());
    if (!hProc || hProc == INVALID_HANDLE_VALUE)
        return false;

    m_process_handle.reset(hProc);

    auto base = get_module_base(m_process_id.value(), L"client.dll");
    if (!base.has_value())
        return false;

    m_client_base = base.value();

    return true;
}