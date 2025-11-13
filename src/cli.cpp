#include "cli.h"

#include <cstdlib>
#include <sstream>
#include <cstring>
#include <string_view>

#include <windows.h>

static bool parseInt(const char *s, int &out)
{
    if (!s)
        return false;
    char *end = nullptr;
    long v = std::strtol(s, &end, 10);
    if (end == s || *end != '\0')
        return false;
    out = static_cast<int>(v);
    return true;
}

static int parseOrientationToken(const char* s)
{
    if (!s) return -1;
    // Accept numbers: 0, 90, 180, 270
    if (std::strcmp(s, "0") == 0) return DMDO_DEFAULT;
    if (std::strcmp(s, "90") == 0) return DMDO_90;
    if (std::strcmp(s, "180") == 0) return DMDO_180;
    if (std::strcmp(s, "270") == 0) return DMDO_270;
    // Accept friendly names
    if (_stricmp(s, "landscape") == 0) return DMDO_DEFAULT;
    if (_stricmp(s, "portrait") == 0) return DMDO_90;
    if (_stricmp(s, "landscape-flipped") == 0 || _stricmp(s, "landscape_inverted") == 0) return DMDO_180;
    if (_stricmp(s, "portrait-flipped") == 0 || _stricmp(s, "portrait_inverted") == 0) return DMDO_270;
    return -1;
}

static bool parseBoolFlag(const char* a, const char* name) {
    return std::strcmp(a, name) == 0;
}

bool drt::parseArgs(int argc, char **argv, drt::Args &out)
{
    if (argc < 1) return false;
    out.program = argv[0];
    for (int i = 1; i < argc; ++i)
    {
        const char *a = argv[i];

        if (parseBoolFlag(a, "--list"))
        {
            out.list = true;
            continue;
        }
        if (parseBoolFlag(a, "--list-modes"))
        {
            out.listModes = true;
            continue;
        }
        if (parseBoolFlag(a, "--persist"))
        {
            out.persist = true;
            continue;
        }
        if (parseBoolFlag(a, "--dry-run"))
        {
            out.dryRun = true;
            continue;
        }
        if (parseBoolFlag(a, "--json"))
        {
            out.json = true;
            continue;
        }
        if (parseBoolFlag(a, "--verbose"))
        {
            out.verbose = true;
            continue;
        }
        if (parseBoolFlag(a, "--quiet"))
        {
            out.quiet = true;
            continue;
        }

        if (std::strcmp(a, "--display") == 0 && i + 1 < argc)
        {
            out.display = argv[++i];
            continue;
        }
        if (std::strcmp(a, "--width") == 0 && i + 1 < argc)
        {
            if (!parseInt(argv[++i], out.width)) return false;
            continue;
        }
        if (std::strcmp(a, "--height") == 0 && i + 1 < argc)
        {
            if (!parseInt(argv[++i], out.height)) return false;
            continue;
        }
        if (std::strcmp(a, "--hz") == 0 && i + 1 < argc)
        {
            if (!parseInt(argv[++i], out.hz)) return false;
            continue;
        }
        if (std::strcmp(a, "--orientation") == 0 && i + 1 < argc)
        {
            int o = parseOrientationToken(argv[++i]);
            if (o < 0)
                return false;
            out.orientation = o;
            continue;
        }

        // unknown token
        return false;
    }

    return true;
}

std::string drt::usage(const std::string &program)
{
    std::ostringstream ss;
    ss << "Usage:\n"
       << "  " << program << " --list [--json]\n"
       << "  " << program << " --list-modes --display <index|name|\\\\.\\DISPLAYn> [--json]\n"
       << "  " << program << " --display <index|name|\\\\.\\DISPLAYn> [--width W --height H] [--hz F] [--orientation (0|90|180|270)] [--persist] [--dry-run] [--json]\n\n"
       << "Options:\n"
       << "  --list                     List active displays.\n"
       << "  --list-modes               List modes for a display (requires --display).\n"
       << "  --display <sel>            Select display by index (from --list), friendly name substring, or source (e.g., \\\\.\\DISPLAY1).\n"
       << "  --width/--height           Target resolution. If only one set, the other must be provided.\n"
       << "  --hz                       Target refresh rate.\n"
       << "  --orientation              0=landscape,90=portrait,180=landscape-flipped,270=portrait-flipped.\n"
       << "  --persist                  Save change to registry (CDS_UPDATEREGISTRY).\n"
       << "  --dry-run                  Validate only (no change).\n"
       << "  --json                     Machine-readable output.\n"
       << "  --verbose                  Extra diagnostics to stderr.\n"
       << "  --quiet                    Suppress human-readable output.\n";
    return ss.str();
}
