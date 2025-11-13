#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace drt {

// Convert wide string to UTF-8 (best-effort)
inline std::string to_utf8(const std::wstring& ws) {
    if (ws.empty()) return {};
    int needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
                                     nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()),
                        out.data(), needed, nullptr, nullptr);
    return out;
}
inline std::string to_utf8(const wchar_t* w) {
    if (!w) return {};
    int needed = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string out(static_cast<size_t>(needed-1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), needed, nullptr, nullptr);
    return out;
}

// Small RAII for change display settings test flag mapping
inline const char* dmOrientationToString(DWORD o) {
    switch (o) {
        case DMDO_DEFAULT: return "landscape";
        case DMDO_90: return "portrait";
        case DMDO_180: return "landscape-flipped";
        case DMDO_270: return "portrait-flipped";
        default: return "unknown";
    }
}

} // namespace drt
