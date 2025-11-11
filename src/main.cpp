#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "cli.h"
#include "windows_display.h"
#include "display_config.h"

int main(int argc, char **argv)
{
    drt::Args a;
    if (!drt::parseArgs(argc, argv, a))
    {
        std::cerr << drt::usage(a.program) << std::endl;
        return EXIT_FAILURE;
    }

    auto resolveSourceName = [](const std::string &sel, std::string &outSource) -> bool {
        // If looks like \\.\DISPLAYn, use directly
        if (sel.rfind("\\\\.\\DISPLAY", 0) == 0) { outSource = sel; return true; }

        // Try numeric index
        char *end = nullptr;
        long idx = std::strtol(sel.c_str(), &end, 10);
        std::vector<drt::DisplayInfo> displays;
        std::string err;
        if (*sel.c_str() != '\0' && *end == '\0') {
            if (!drt::listDisplays(displays, err)) return false;
            if (idx < 0 || static_cast<size_t>(idx) >= displays.size()) return false;
            outSource = displays[static_cast<size_t>(idx)].sourceName;
            return true;
        }
        // Substring match on friendly or source
        if (!drt::listDisplays(displays, err)) return false;
        int match = -1;
        for (size_t i = 0; i < displays.size(); ++i) {
            const auto &d = displays[i];
            if (d.friendlyName.find(sel) != std::string::npos || d.sourceName.find(sel) != std::string::npos) {
                if (match != -1) return false; // ambiguous
                match = static_cast<int>(i);
            }
        }
        if (match == -1) return false;
        outSource = displays[static_cast<size_t>(match)].sourceName;
        return true;
    };

    // Listing flows
    if (a.list || (a.listModes && !a.display.empty()))
    {
        if (a.list)
        {
            std::vector<drt::DisplayInfo> displays;
            std::string err;
            if (!drt::listDisplays(displays, err))
            {
                std::cerr << (a.quiet ? "" : err) << std::endl;
                return 5;
            }
            if (a.json)
            {
                std::cout << "[";
                for (size_t i = 0; i < displays.size(); ++i)
                {
                    const auto &d = displays[i];
                    std::cout << (i ? "," : "")
                              << "{\"index\":" << i
                              << ",\"name\":\"" << d.friendlyName << "\""
                              << ",\"source\":\"" << d.sourceName << "\"";
                    std::cout << ",\"primary\":" << (d.isPrimary ? "true" : "false") << "}";
                }
                std::cout << "]\n";
            }
            else
            {
                for (size_t i = 0; i < displays.size(); ++i)
                {
                    const auto &d = displays[i];
                    std::cout << i << ": " << d.friendlyName << " [" << d.sourceName << "]" << (d.isPrimary ? " *" : "") << "\n";
                }
            }
            return 0;
        }

        if (a.listModes)
        {
            std::string sourceName = a.display;
            if (!resolveSourceName(a.display, sourceName))
            {
                std::cerr << (a.quiet ? "" : "Display not found or ambiguous") << std::endl;
                return 3;
            }
            std::vector<drt::ModeInfo> modes;
            std::string err;
            if (!drt::listModes(sourceName, modes, err))
            {
                std::cerr << (a.quiet ? "" : err) << std::endl;
                return 5;
            }
            if (a.json)
            {
                std::cout << "[";
                for (size_t i = 0; i < modes.size(); ++i)
                {
                    const auto &m = modes[i];
                    std::cout << (i ? "," : "")
                              << "{\"w\":" << m.width
                              << ",\"h\":" << m.height
                              << ",\"hz\":" << m.hz
                              << ",\"bpp\":" << m.bitsPerPel << "}";
                }
                std::cout << "]\n";
            }
            else
            {
                for (const auto &m : modes)
                {
                    std::cout << m.width << "x" << m.height << " @ " << m.hz << "Hz";
                    if (m.bitsPerPel) std::cout << " (" << m.bitsPerPel << "bpp)";
                    std::cout << "\n";
                }
            }
            return 0;
        }
    }

    // Apply flow
    if (a.display.empty() || (a.hz < 0 && a.width < 0 && a.height < 0 && a.orientation < 0))
    {
        std::cerr << drt::usage(a.program) << std::endl;
        return 4; // invalid args
    }

    std::string sourceName;
    if (!resolveSourceName(a.display, sourceName))
    {
        std::cerr << (a.quiet ? "" : "Display not found or ambiguous") << std::endl;
        return 3;
    }

    drt::ApplyRequest req;
    req.sourceName = sourceName;
    req.width = a.width;
    req.height = a.height;
    req.hz = a.hz;
    req.orientation = a.orientation;
    req.persist = a.persist;
    req.dryRun = a.dryRun;

    drt::ApplyResult res;
    if (!drt::applyMode(req, res))
    {
        if (a.json)
        {
            std::cout << "{\"success\":false,\"message\":\"" << res.message << "\"}\n";
        }
        else if (!a.quiet)
        {
            std::cerr << res.message << std::endl;
        }
        return 6;
    }

    if (a.json)
    {
        std::cout << "{\"success\":" << (res.success ? "true" : "false")
                  << ",\"changed\":" << (res.changed ? "true" : "false")
                  << ",\"message\":\"" << res.message << "\"}\n";
    }
    else if (!a.quiet)
    {
        std::cout << (a.dryRun ? "Validated: " : "Applied: ") << res.message << std::endl;
    }
    return res.success ? (res.changed ? 0 : 2) : 6;
}
