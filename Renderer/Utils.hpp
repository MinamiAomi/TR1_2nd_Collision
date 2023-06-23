#pragma once
#include <winerror.h>
#include <WinUser.h>

#define COM_RESULT(function) ComResult(function, #function)

inline void ComResult(HRESULT hr, const char* text) {
    if (SUCCEEDED(hr)) { 
        return;
    }

    const HRESULT error_table[]{
        E_NOTIMPL,
        E_NOINTERFACE,
        E_POINTER,
        E_ABORT,
        E_FAIL,
        E_UNEXPECTED,
        E_ACCESSDENIED,
        E_HANDLE,
        E_OUTOFMEMORY,
        E_INVALIDARG };

    const char* error_msg[]{
        "Not implemented",
        "No such interface supported",
        "Invalid pointer",
        "Operation aborted",
        "Unspecified error",
        "Catastrophic failure",
        "General access denied error",
        "Invalid handle",
        "Ran out of memory",
        "One or more arguments are invalid" };

    for (int i = 0; i < ARRAYSIZE(error_table); i++) {
        if (hr == error_table[i]) {
            MessageBoxA(nullptr, text, error_msg[i], MB_OK | MB_ICONERROR);
            break; 
        }
    }
    std::abort();
}