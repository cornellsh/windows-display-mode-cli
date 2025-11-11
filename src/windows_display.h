#pragma once

#include <string>
#include <windows.h>

namespace drt
{
    bool getAdapterByIndex(int index, DISPLAY_DEVICEA &adapter);

    std::string changeResultToText(LONG code);

    void printMonitors(const DISPLAY_DEVICEA &adapter);

    bool setRefreshHz(const DISPLAY_DEVICEA &adapter, int hz, std::string &error_message);

} // namespace drt
