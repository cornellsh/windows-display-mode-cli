#include "display_config.h"
#include "util.h"

#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>

static bool getTargetFriendlyName(const DISPLAYCONFIG_PATH_INFO& path, std::string& out) {
    DISPLAYCONFIG_TARGET_DEVICE_NAME name = {};
    name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    name.header.size = sizeof(name);
    name.header.adapterId = path.targetInfo.adapterId;
    name.header.id = path.targetInfo.id;
    if (DisplayConfigGetDeviceInfo(&name.header) != ERROR_SUCCESS) return false;

    out = drt::to_utf8(name.monitorFriendlyDeviceName);
    return !out.empty();
}

static bool getSourceDeviceName(const DISPLAYCONFIG_PATH_INFO& path, std::string& out) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME src = {};
    src.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    src.header.size = sizeof(src);
    src.header.adapterId = path.sourceInfo.adapterId;
    src.header.id = path.sourceInfo.id;
    if (DisplayConfigGetDeviceInfo(&src.header) != ERROR_SUCCESS) return false;
    out = drt::to_utf8(src.viewGdiDeviceName);
    return !out.empty();
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
        info.isPrimary = (p.sourceInfo.id == 0);
        getTargetFriendlyName(p, info.friendlyName);
        getSourceDeviceName(p, info.sourceName);
        out.push_back(info);
    }
    if (out.empty()) {
        errorMessage = "No active displays found";
        return false;
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
    // Sort unique by width, height, hz
    std::sort(out.begin(), out.end(), [](const ModeInfo& a, const ModeInfo& b){
        if (a.width != b.width) return a.width < b.width;
        if (a.height != b.height) return a.height < b.height;
        if (a.hz != b.hz) return a.hz < b.hz;
        return a.orientation < b.orientation;
    });
    out.erase(std::unique(out.begin(), out.end(), [](const ModeInfo& a, const ModeInfo& b){
        return a.width==b.width && a.height==b.height && a.hz==b.hz && a.orientation==b.orientation;
    }), out.end());
    return true;
}

static void copyDevModeFromRequest(const drt::ApplyRequest& req, DEVMODEA& dm, const DEVMODEA& current, bool& hasChange) {
    dm = current;
    dm.dmFields = 0;
    hasChange = false;

    if (req.width > 0 && req.height > 0) {
        dm.dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPelsWidth = static_cast<DWORD>(req.width);
        dm.dmPelsHeight = static_cast<DWORD>(req.height);
        hasChange = true;
    }
    if (req.hz > 0) {
        dm.dmFields |= DM_DISPLAYFREQUENCY;
        dm.dmDisplayFrequency = static_cast<DWORD>(req.hz);
        hasChange = true;
    }
    if (req.orientation >= 0) {
        dm.dmFields |= DM_DISPLAYORIENTATION;
        dm.dmDisplayOrientation = static_cast<DWORD>(req.orientation);
        hasChange = true;
    }
}

bool drt::applyMode(const drt::ApplyRequest& req, drt::ApplyResult& result) {
    result = {};
    if (req.sourceName.empty()) {
        result.message = "No sourceName provided";
        return false;
    }
    DEVMODEA current = {};
    current.dmSize = sizeof(current);
    if (!EnumDisplaySettingsA(req.sourceName.c_str(), ENUM_CURRENT_SETTINGS, &current)) {
        result.message = "Failed to read current settings";
        return false;
    }

    DEVMODEA target = {};
    bool willChange = false;
    copyDevModeFromRequest(req, target, current, willChange);

    if (!willChange) {
        result.success = true;
        result.changed = false;
        result.message = "No change requested";
        return true;
    }

    DWORD flags = req.persist ? CDS_UPDATEREGISTRY : 0;
    if (req.dryRun) flags |= CDS_TEST;

    LONG ch = ChangeDisplaySettingsExA(req.sourceName.c_str(), &target, nullptr, flags, nullptr);
    if (ch != DISP_CHANGE_SUCCESSFUL) {
        result.success = false;
        result.changed = false;
        switch (ch) {
            case DISP_CHANGE_BADMODE: result.message = "Unsupported mode"; break;
            case DISP_CHANGE_RESTART: result.message = "Restart required"; break;
            case DISP_CHANGE_BADPARAM: result.message = "Bad parameter"; break;
            case DISP_CHANGE_FAILED: result.message = "Driver rejected mode"; break;
            default: result.message = "ChangeDisplaySettingsEx error code " + std::to_string(ch); break;
        }
        return false;
    }

    if (req.dryRun) {
        result.success = true;
        result.changed = false;
        result.message = "Valid (dry-run)";
        return true;
    }

    // Verify by re-reading current
    DEVMODEA after = {};
    after.dmSize = sizeof(after);
    if (!EnumDisplaySettingsA(req.sourceName.c_str(), ENUM_CURRENT_SETTINGS, &after)) {
        result.success = true; // applied, but verification not possible
        result.changed = true;
        result.message = "Applied (verification read failed)";
        return true;
    }
    bool ok = true;
    if ((target.dmFields & DM_PELSWIDTH) && after.dmPelsWidth != target.dmPelsWidth) ok = false;
    if ((target.dmFields & DM_PELSHEIGHT) && after.dmPelsHeight != target.dmPelsHeight) ok = false;
    if ((target.dmFields & DM_DISPLAYFREQUENCY) && after.dmDisplayFrequency != target.dmDisplayFrequency) ok = false;
    if ((target.dmFields & DM_DISPLAYORIENTATION) && after.dmDisplayOrientation != target.dmDisplayOrientation) ok = false;

    if (!ok) {
        result.success = false;
        result.changed = false;
        result.message = "Verification mismatch after apply";
        return false;
    }

    result.success = true;
    result.changed = true;
    result.message = req.persist ? "Applied and persisted" : "Applied (session only)";
    return true;
}
