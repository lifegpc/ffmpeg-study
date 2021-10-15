#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif

#include "getopt.h"
#include "wchar_util.h"
#include <stdio.h>
#include <string.h>
#include "enm4a.h"

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
        --trace             Enable trace logging.\n");
}

#define ENM4A_TRACE 129

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
        {"trace", 0, nullptr, ENM4A_TRACE},
        nullptr,
    };
    int c;
    const char* shortopts = "-ho:vd";
    std::string output = "";
    std::string input = "";
    ENM4A_LOG level = ENM4A_LOG_INFO;
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
            level = ENM4A_LOG_VERBOSE;
            break;
        case 'd':
            level = ENM4A_LOG_DEBUG;
            break;
        case ENM4A_TRACE:
            level = ENM4A_LOG_TRACE;
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
    if (level >= ENM4A_LOG_VERBOSE) {
        printf("Get input file name: %s\n", input.c_str());
    }
    ENM4A_ARGS arg;
    memset(&arg, 0, sizeof(ENM4A_ARGS));
    arg.level = level;
    size_t olen = output.length();
    if (olen) {
        arg.output = (char*)malloc(olen + 1);
        if (!arg.output) {
            printf("%s\n", enm4a_error_msg(ENM4A_NO_MEMORY));
            return 1;
        }
        memcpy(arg.output, output.c_str(), olen);
        arg.output[olen] = 0;
    }
    ENM4A_ERROR re = encode_m4a(input.c_str(), arg);
    if (arg.output) {
        free(arg.output);
    }
    if (re != ENM4A_OK) {
        if (re != ENM4A_FFMPEG_ERR) {
            printf("%s\n", enm4a_error_msg(re));
        }
        return 1;
    }
    return 0;
}
