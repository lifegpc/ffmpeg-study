#if HAVE_FFCONCAT_CONFIG_H
#include "ffconcat_config.h"
#endif

#include "getopt.h"
#include "wchar_util.h"
#include <list>
#include "ffconcat.h"
#include "fileop.h"
#include <stdio.h>

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_help() {
    printf("%s", "Usage: ffconcat [options] [FILE [..]]\n\
Concat video files\n\
\n\
Options:\n\
    -h, --help              Print help message.\n\
    -o, --output [FILE]     Specifiy output file location. Default output location: a.mp4.\n\
    -v, --verbose           Enable verbose logging.\n\
    -d, --debug             Enable debug logging.\n\
    -t, --trace             Enable trace logging.\n");
}

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char **wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
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
    std::string output = "a.mp4";
    std::list<std::string> li;
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
                li.push_back(optarg);
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
    if (verbose) {
        printf("Output file: %s\n", output.c_str());
        for(auto i = li.begin(); i != li.end(); i++) {
            printf("Input file: %s\n", (*i).c_str());
        }
    }
    if (fileop::exists(output)) {
        printf("Output file already exists, do you want to overwrite it? (y/n)");
        int c = getchar();
        while (c != 'y' && c != 'n') {
            c = getchar();
        }
        if (c == 'n') {
            return 0;
        }
        if (!fileop::remove(output, true)) {
            return 1;
        }
    }
    ffconcath conf;
    conf.verbose = verbose;
    conf.debug = debug;
    conf.trace = trace;
    int re = ffconcat(output, li, conf);
    return re;
}
