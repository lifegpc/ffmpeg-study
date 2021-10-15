#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "getopt.h"
#include "wchar_util.h"
#include <stdio.h>

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_help() {
    printf("%s", "Usage: enm4a [options] FILE\n\
Convert to m4a file\n\
\n\
Options:\n\
    -h, --help              Print help message.\n\
    -o, --output [FILE]     Specifiy output file location. Default output location: <title>.m4a.\n\
                            If title is not found, use \"a\" instead.\n\
    -v, --verbose           Enable verbose logging.\n\
    -d, --debug             Enable debug logging.\n\
    -t, --trace             Enable trace logging.\n");
}

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char** wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
    if (argc == 1) {
        print_help();
#if _WIN32
        if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
        return 0;
    }
    struct option opts[] = {
        {"help", 0, nullptr, 'h'},
        {"output", 1, nullptr, 'o'},
        {"verbose", 0, nullptr, 'v'},
        {"debug", 0, nullptr, 'd'},
        {"trace", 0, nullptr, 't'},
        nullptr,
    };
    int c;
    const char* shortopts = "-ho:vdt";
    std::string output = "";
    std::string input = "";
    bool verbose = false;
    bool debug = false;
    bool trace = false;
    while ((c = getopt_long(argc, argv, shortopts, opts, nullptr)) != -1) {
        switch (c) {
        case 'h':
            print_help();
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 0;
        case 'o':
            output = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        case 'd':
            debug = true;
            verbose = true;
            break;
        case 't':
            trace = true;
            debug = true;
            verbose = true;
            break;
        case 1:
            if (!input.length()) {
                input = optarg;
            } else {
                printf("%s\n", "Too much input files.");
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
            break;
        case '?':
        default:
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 1;
        }
    }
#if _WIN32
    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
    if (!input.length()) {
        printf("%s\n", "An input file is needed.");
        return 1;
    }
    return 0;
}
