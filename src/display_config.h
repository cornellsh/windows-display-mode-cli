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
    std::string friendlyName;   // Target friendly name
    std::string sourceName;     // e.g. \\.\DISPLAY1
    bool isPrimary = false;
};

struct ModeInfo {
    int width = 0;
    int height = 0;
    int hz = 0; // integer Hz for now
    int orientation = DMDO_DEFAULT; // DMDO_*
    int bitsPerPel = 0;
};

struct ApplyRequest {
    // target selection by source name (e.g., \\.\DISPLAY1)
    std::string sourceName;
    int width = -1;
    int height = -1;
    int hz = -1; // integer Hz; decimals will be rounded by CLI
    int orientation = -1; // DMDO_*
    bool persist = false; // save to registry
    bool dryRun = false;  // validation only
};

struct ApplyResult {
    bool changed = false;
    bool success = false;
    std::string message;
};

// Query active displays with stable identifiers and names.
bool listDisplays(std::vector<DisplayInfo>& out, std::string& errorMessage);

// Enumerate available modes for a given source device name (e.g., \\.\DISPLAY1).
bool listModes(const std::string& sourceName, std::vector<ModeInfo>& out, std::string& errorMessage);

// Apply a mode change using Win32 Display Settings API with validation and optional persistence.
bool applyMode(const ApplyRequest& req, ApplyResult& result);

} // namespace drt

