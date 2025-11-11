#include "cli.h"

#include <cstdlib>
#include <sstream>
#include <cstring>

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

std::string drt::usage(const std::string &program)
{
    std::ostringstream ss;
    ss << "Usage:\n\n"
       << "  " << program << " --list [--json]\n"
       << "  " << program << " --list-modes --display <id|index|name> [--json]\n"
       << "  " << program << " --display <id|index|name> [--hz HZ] [--width W] [--height H] [--orientation landscape|portrait|landscape_flipped|portrait_flipped] [--persist] [--dry-run] [--json]\n\n"
       << "Notes:\n"
       << "  - --display accepts Windows source name (e.g., \\ \\.\\DISPLAY1), numeric index, or a name substring.\n"
       << "  - --hz accepts integer or decimal; decimals are rounded.\n";
    return ss.str();
}

static bool parseBoolFlag(const char *a, const char *name)
{
    return std::strcmp(a, name) == 0;
}

static int parseOrientationToken(const char *token)
{
    if (!token)
        return -1;
    if (std::strcmp(token, "landscape") == 0)
        return DMDO_DEFAULT;
    if (std::strcmp(token, "portrait") == 0)
        return DMDO_90;
    if (std::strcmp(token, "landscape_flipped") == 0)
        return DMDO_180;
    if (std::strcmp(token, "portrait_flipped") == 0)
        return DMDO_270;
    return -1;
}

bool drt::parseArgs(int argc, char **argv, drt::Args &out)
{
    out.program = argv && argv[0] ? argv[0] : "displaymode";
    if (argc <= 1)
        return false;

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
            int v = 0;
            if (parseInt(argv[++i], v))
                out.width = v;
            else
                return false;
            continue;
        }
        if (std::strcmp(a, "--height") == 0 && i + 1 < argc)
        {
            int v = 0;
            if (parseInt(argv[++i], v))
                out.height = v;
            else
                return false;
            continue;
        }
        if (std::strcmp(a, "--hz") == 0 && i + 1 < argc)
        {
            // accept decimal; round to nearest int
            const char *s = argv[++i];
            char *end = nullptr;
            double dv = std::strtod(s, &end);
            if (end == s)
                return false;
            out.hz = static_cast<int>(dv + (dv >= 0 ? 0.5 : -0.5));
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
