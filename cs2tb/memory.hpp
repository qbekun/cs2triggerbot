#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <optional>
#include <string>
#include <memory>
#include <iostream>

struct handle_disposer_t {
    using pointer = HANDLE;
    void operator()(HANDLE handle) const {
        if (handle && handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }
};

using unique_handle = std::unique_ptr<HANDLE, handle_disposer_t>;

class c_game {
private:
    std::optional<DWORD> m_process_id;
    unique_handle m_process_handle{ nullptr };
    std::uintptr_t m_client_base = 0;

public:
    bool initialize();
    std::uintptr_t get_client_base() const noexcept { return m_client_base; }

    template <typename T>
    std::optional<T> read(const std::uintptr_t& address) const noexcept {
        if (!m_process_handle) return std::nullopt;
        T value{};
        if (ReadProcessMemory(m_process_handle.get(), reinterpret_cast<const void*>(address), &value, sizeof(T), nullptr))
            return value;
        return std::nullopt;
    }
};
