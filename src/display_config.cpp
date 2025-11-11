#include "display_config.h"

#include <vector>
#include <string>
#include <cstring>
#include <sstream>

static bool getTargetFriendlyName(const DISPLAYCONFIG_PATH_INFO& path, std::string& out) {
    DISPLAYCONFIG_TARGET_DEVICE_NAME name = {};
    name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    name.header.size = sizeof(name);
    name.header.adapterId = path.targetInfo.adapterId;
    name.header.id = path.targetInfo.id;
    if (DisplayConfigGetDeviceInfo(&name.header) != ERROR_SUCCESS) return false;

    // convert WCHAR to UTF-8 (simple narrowing, best-effort)
    char buffer[256] = {};
    int len = WideCharToMultiByte(CP_UTF8, 0, name.monitorFriendlyDeviceName, -1, buffer, sizeof(buffer)-1, nullptr, nullptr);
    if (len <= 0) return false;
    out.assign(buffer);
    return true;
}

static bool getSourceDeviceName(const DISPLAYCONFIG_PATH_INFO& path, std::string& out) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME src = {};
    src.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    src.header.size = sizeof(src);
    src.header.adapterId = path.sourceInfo.adapterId;
    src.header.id = path.sourceInfo.id;
    if (DisplayConfigGetDeviceInfo(&src.header) != ERROR_SUCCESS) return false;
    char buffer[128] = {};
    int len = WideCharToMultiByte(CP_UTF8, 0, src.viewGdiDeviceName, -1, buffer, sizeof(buffer)-1, nullptr, nullptr);
    if (len <= 0) return false;
    out.assign(buffer);
    return true;
}

bool drt::listDisplays(std::vector<drt::DisplayInfo>& out, std::string& errorMessage) {
    UINT32 pathCount = 0, modeCount = 0;
    LONG st = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
    if (st != ERROR_SUCCESS) { errorMessage = "GetDisplayConfigBufferSizes failed"; return false; }
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
    st = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);
    if (st != ERROR_SUCCESS) { errorMessage = "QueryDisplayConfig failed"; return false; }

    out.clear();
    out.reserve(pathCount);
    for (UINT32 i = 0; i < pathCount; ++i) {
        const auto& p = paths[i];
        DisplayInfo info;
        info.id.adapterLuid = p.targetInfo.adapterId;
        info.id.targetId = p.targetInfo.id;
        info.isPrimary = (p.sourceInfo.statusFlags & DISPLAYCONFIG_SOURCE_IN_USE) != 0 && (p.sourceInfo.id == 0);
        getTargetFriendlyName(p, info.friendlyName);
        getSourceDeviceName(p, info.sourceName);
        out.push_back(info);
    }
    return true;
}

bool drt::listModes(const std::string& sourceName, std::vector<drt::ModeInfo>& out, std::string& errorMessage) {
    out.clear();
    // Enumerate via EnumDisplaySettings on the source device (e.g., \\.\DISPLAY1)
    DEVMODEA dm = {};
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsA(sourceName.c_str(), i, &dm); ++i) {
        ModeInfo m;
        m.width = static_cast<int>(dm.dmPelsWidth);
        m.height = static_cast<int>(dm.dmPelsHeight);
        m.hz = static_cast<int>(dm.dmDisplayFrequency);
        m.orientation = dm.dmDisplayOrientation;
        m.bitsPerPel = static_cast<int>(dm.dmBitsPerPel);
        out.push_back(m);
    }
    if (out.empty()) {
        errorMessage = "No modes or failed to enumerate for " + sourceName;
        return false;
    }
    return true;
}

bool drt::applyMode(const drt::ApplyRequest& req, drt::ApplyResult& result) {
    result = {};
    DEVMODEA current = {};
    current.dmSize = sizeof(current);
    if (!EnumDisplaySettingsA(req.sourceName.c_str(), ENUM_CURRENT_SETTINGS, &current)) {
        result.message = "Failed to read current settings";
        return false;
    }

    DEVMODEA target = current;
    target.dmFields = 0;
    if (req.width > 0) { target.dmPelsWidth = req.width; target.dmFields |= DM_PELSWIDTH; }
    if (req.height > 0) { target.dmPelsHeight = req.height; target.dmFields |= DM_PELSHEIGHT; }
    if (req.hz > 0) { target.dmDisplayFrequency = req.hz; target.dmFields |= DM_DISPLAYFREQUENCY; }
    if (req.orientation >= 0) { target.dmDisplayOrientation = req.orientation; target.dmFields |= DM_DISPLAYORIENTATION; }

    // No-op detection
    bool wouldChange = false;
    if ((target.dmFields & DM_PELSWIDTH) && target.dmPelsWidth != current.dmPelsWidth) wouldChange = true;
    if ((target.dmFields & DM_PELSHEIGHT) && target.dmPelsHeight != current.dmPelsHeight) wouldChange = true;
    if ((target.dmFields & DM_DISPLAYFREQUENCY) && target.dmDisplayFrequency != current.dmDisplayFrequency) wouldChange = true;
    if ((target.dmFields & DM_DISPLAYORIENTATION) && target.dmDisplayOrientation != current.dmDisplayOrientation) wouldChange = true;
    if (!wouldChange) {
        result.success = true;
        result.changed = false;
        result.message = "Already in requested state";
        return true;
    }

    DWORD flags = 0;
    if (req.dryRun) flags |= CDS_TEST;
    if (req.persist) flags |= CDS_UPDATEREGISTRY;

    LONG r = ChangeDisplaySettingsExA(req.sourceName.c_str(), &target, nullptr, flags, nullptr);
    if (r != DISP_CHANGE_SUCCESSFUL) {
        std::ostringstream ss;
        ss << "apply failed (code=" << r << ")";
        result.message = ss.str();
        return false;
    }

    // If not dry-run, verify by re-reading
    if (!req.dryRun) {
        DEVMODEA after = {};
        after.dmSize = sizeof(after);
        if (!EnumDisplaySettingsA(req.sourceName.c_str(), ENUM_CURRENT_SETTINGS, &after)) {
            result.message = "Applied but could not verify";
            result.success = true;
            result.changed = true;
            return true;
        }
        bool ok = true;
        if ((target.dmFields & DM_PELSWIDTH) && after.dmPelsWidth != target.dmPelsWidth) ok = false;
        if ((target.dmFields & DM_PELSHEIGHT) && after.dmPelsHeight != target.dmPelsHeight) ok = false;
        if ((target.dmFields & DM_DISPLAYFREQUENCY) && after.dmDisplayFrequency != target.dmDisplayFrequency) ok = false;
        if ((target.dmFields & DM_DISPLAYORIENTATION) && after.dmDisplayOrientation != target.dmDisplayOrientation) ok = false;
        if (!ok) {
            result.message = "Verification mismatch after apply";
            return false;
        }
    }

    result.success = true;
    result.changed = !req.dryRun; // treat dry-run success as not changed
    result.message = req.dryRun ? "Valid (dry-run)" : "Applied";
    return true;
}

