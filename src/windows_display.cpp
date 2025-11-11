#include "windows_display.h"

#include <iostream>
#include <cstring>

bool drt::getAdapterByIndex(int index, DISPLAY_DEVICEA &adapter)
{
    std::memset(&adapter, 0, sizeof(adapter));
    adapter.cb = sizeof(adapter);

    DISPLAY_DEVICEA tmp;
    for (int i = 0;; ++i)
    {
        std::memset(&tmp, 0, sizeof(tmp));
        tmp.cb = sizeof(tmp);
        if (!EnumDisplayDevicesA(nullptr, i, &tmp, DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            break;
        }
        if (i == index)
        {
            adapter = tmp;
            return true;
        }
    }
    return false;
}

std::string drt::changeResultToText(LONG code)
{
    switch (code)
    {
    case DISP_CHANGE_SUCCESSFUL:
        return "settings applied";
    case DISP_CHANGE_RESTART:
        return "restart required";
    case DISP_CHANGE_BADMODE:
        return "mode unsupported";
    case DISP_CHANGE_BADPARAM:
        return "bad parameter";
    case DISP_CHANGE_BADFLAGS:
        return "invalid flags";
    case DISP_CHANGE_BADDUALVIEW:
        return "dual-view limitation";
    case DISP_CHANGE_NOTUPDATED:
        return "registry not updated";
    case DISP_CHANGE_FAILED:
        return "driver rejected mode";
    default:
        return "unknown error";
    }
}

void drt::printMonitors(const DISPLAY_DEVICEA &adapter)
{
    for (int m = 0;; ++m)
    {
        DISPLAY_DEVICEA mon;
        std::memset(&mon, 0, sizeof(mon));
        mon.cb = sizeof(mon);
        if (!EnumDisplayDevicesA(adapter.DeviceName, m, &mon, DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            break;
        }
        std::cout << "  " << mon.DeviceString << "\n";
    }
}

bool drt::setRefreshHz(const DISPLAY_DEVICEA &adapter, int hz, std::string &error_message)
{
    DEVMODEA dm;
    std::memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    if (!EnumDisplaySettingsA(adapter.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
    {
        error_message = "could not read current display mode";
        return false;
    }

    dm.dmFields = DM_DISPLAYFREQUENCY;
    dm.dmDisplayFrequency = hz;

    LONG r = ChangeDisplaySettingsExA(adapter.DeviceName, &dm, nullptr, CDS_UPDATEREGISTRY, nullptr);
    if (r != DISP_CHANGE_SUCCESSFUL)
    {
        error_message = drt::changeResultToText(r);
        return false;
    }
    return true;
}
