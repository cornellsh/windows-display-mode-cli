#pragma once
#include <string>

namespace drt
{
    struct Args
    {
        std::string program;

        // operations
        bool list = false;           // --list
        bool listModes = false;      // --list-modes

        // target selection
        std::string display;         // --display <id|index|name>

        // requested mode parameters
        int width = -1;              // --width
        int height = -1;             // --height
        int hz = -1;                 // --hz
        int orientation = -1;        // --orientation (DMDO_*)

        // behavior flags
        bool persist = false;        // --persist
        bool dryRun = false;         // --dry-run
        bool json = false;           // --json
        bool verbose = false;        // --verbose
        bool quiet = false;          // --quiet
    };

    bool parseArgs(int argc, char **argv, Args &out);
    std::string usage(const std::string &program);

} // namespace drt
