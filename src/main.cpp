#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

#include "cli.h"
#include "windows_display.h"
#include "display_config.h"
#include "util.h"
#include "version.h"

static bool parseIndex(const std::string& s, int& out) {
    if (s.empty()) return false;
    for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    out = std::stoi(s);
    return true;
}

int main(int argc, char **argv)
{
    drt::Args a;
    if (!drt::parseArgs(argc, argv, a))
    {
        std::cerr << drt::usage(a.program) << std::endl;
        return EXIT_FAILURE;
    }

    auto resolveSourceName = [&](const std::string &sel, std::string &outSource) -> bool
    {
        if (sel.rfind(R"(\\.\DISPLAY)", 0) == 0) { outSource = sel; return true; }
        int idx = -1;
        if (parseIndex(sel, idx)) {
            std::vector<drt::DisplayInfo> displays;
            std::string err;
            if (!drt::listDisplays(displays, err)) return false;
            if (idx < 0 || static_cast<size_t>(idx) >= displays.size()) return false;
            outSource = displays[static_cast<size_t>(idx)].sourceName;
            return true;
        }
        // Substring match on friendly or source
        std::vector<drt::DisplayInfo> displays;
        std::string err;
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
            if (a.json) {
                std::cout << "[";
                for (size_t i = 0; i < displays.size(); ++i) {
                    const auto &d = displays[i];
                    std::cout << (i? ",":"") << "{\"index\":" << i
                              << ",\"source\":\"" << d.sourceName << "\""
                              << ",\"name\":\"" << d.friendlyName << "\""
                              << ",\"primary\":" << (d.isPrimary? "true":"false")
                              << "}";
                }
                std::cout << "]\n";
            } else if (!a.quiet)
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
                    std::cout << (i? ",":"") << "{\"width\":" << m.width
                              << ",\"height\":" << m.height
                              << ",\"hz\":" << m.hz
                              << ",\"orientation\":" << m.orientation
                              << ",\"bpp\":" << m.bitsPerPel
                              << "}";
                }
                std::cout << "]\n";
            }
            else if (!a.quiet)
            {
                for (const auto &m : modes)
                {
                    std::cout << m.width << "x" << m.height << "@" << m.hz
                              << " bpp=" << m.bitsPerPel
                              << " orientation=" << m.orientation << "\n";
                }
            }
            return 0;
        }
    }

    // Apply flow
    if (a.display.empty())
    {
        std::cerr << (a.quiet ? "" : "No display selected. Use --display or --list.") << std::endl;
        return 3;
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
            std::cout << "{\"success\":false,\"changed\":false,\"message\":\"" << res.message << "\"}\n";
        }
        else if (!a.quiet)
        {
            std::cerr << "Failed: " << res.message << std::endl;
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
