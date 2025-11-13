#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace drt {

struct DisplayId {
    LUID adapterLuid{};
    UINT32 targetId = 0;
};

struct DisplayInfo {
    DisplayId id;
    std::string friendlyName;   // Monitor friendly name (UTF-8)
    std::string sourceName;     // GDI source name: \\.\DISPLAY1
    bool isPrimary = false;
};

struct ModeInfo {
    int width = 0;
    int height = 0;
    int hz = 0;
    int orientation = 0;     // DMDO_*
    int bitsPerPel = 0;
};

struct ApplyRequest {
    std::string sourceName;    // Required: \\.\DISPLAYn
    int width = -1;
    int height = -1;
    int hz = -1;
    int orientation = -1;      // DMDO_*
    bool persist = false;
    bool dryRun = false;
};

struct ApplyResult {
    bool success = false;
    bool changed = false;
    std::string message;
};

// Query active displays with stable identifiers and names.
bool listDisplays(std::vector<DisplayInfo>& out, std::string& errorMessage);

// Enumerate available modes for a given source device name (e.g., \\.\DISPLAY1).
bool listModes(const std::string& sourceName, std::vector<ModeInfo>& out, std::string& errorMessage);

// Apply a mode change using Win32 Display Settings API with validation and optional persistence.
bool applyMode(const ApplyRequest& req, ApplyResult& result);

} // namespace drt

