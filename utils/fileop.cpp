#if HAVE_UTILS_CONFIG_H
#include "utils_config.h"
#endif

#include "fileop.h"

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include "err.h"
#include "wchar_util.h"
#include <regex>

#ifdef _WIN32
#if HAVE__ACCESS_S
#define access _access_s
#endif
#if HAVE__WACCESS_S
#define _waccess _waccess_s
#endif
#if HAVE_PRINTF_S
#define printf printf_s
#endif
#if HAVE_SSCANF_S
#define sscanf sscanf_s
#endif
#endif

#ifdef _WIN32
bool exists_internal(wchar_t* fn) {
    return !_waccess(fn, 0);
}

bool remove_internal(wchar_t* fn, bool print_error) {
    int ret = _wremove(fn);
    if (ret && print_error && errno != ENOENT) {
        std::string o;
        std::string tfn;
        if (err::get_errno_message(o, errno) && wchar_util::wstr_to_str(tfn, fn, CP_UTF8)) {
            printf("Can not remove file \"%s\": %s.\n", tfn.c_str(), o.c_str());
        }
    }
    return !ret;
}

template <typename T, typename ... Args>
T fileop_internal(const char* fname, UINT codePage, T(*callback)(wchar_t* fn, Args... args), T failed, Args... args) {
    int wlen;
    wchar_t* fn;
    DWORD opt = wchar_util::getMultiByteToWideCharOptions(MB_ERR_INVALID_CHARS, codePage);
    wlen = MultiByteToWideChar(codePage, opt, fname, -1, NULL, 0);
    if (!wlen) return failed;
    fn = (wchar_t*)malloc(sizeof(wchar_t) * wlen);
    if (!MultiByteToWideChar(codePage, opt, fname, -1, fn, wlen)) {
        free(fn);
        return failed;
    }
    T re = callback(fn, args...);
    free(fn);
    return re;
}
#endif

bool fileop::exists(std::string fn) {
#if _WIN32
    UINT cp[] = { CP_UTF8, CP_OEMCP, CP_ACP };
    int i;
    for (i = 0; i < 3; i++) {
        if (fileop_internal(fn.c_str(), cp[i], &exists_internal, false)) return true;
    }
    return !access(fn.c_str(), 0);
#else
    return !access(fn.c_str(), 0);
#endif
}

bool fileop::remove(std::string fn, bool print_error) {
#if _WIN32
    UINT cp[] = { CP_UTF8, CP_OEMCP, CP_ACP };
    int i;
    for (i = 0; i < 3; i++) {
        if (fileop_internal(fn.c_str(), cp[i], &remove_internal, false, print_error)) return true;
    }
#endif
    int ret = ::remove(fn.c_str());
    if (ret && print_error) {
        std::string o;
        if (err::get_errno_message(o, errno)) {
            printf("Can not remove file \"%s\": %s.\n", fn.c_str(), o.c_str());
        }
    }
    return !ret;
}

std::string fileop::dirname(std::string fn) {
    auto i = fn.find_last_of('/');
    auto i2 = fn.find_last_of('\\');
    i = (i == std::string::npos || (i2 != std::string::npos && i2 > i)) ? i2 : i;
    return i == std::string::npos ? "" : fn.substr(0, i);
}

bool fileop::is_url(std::string fn) {
    return fn.find("://") != std::string::npos;
}

std::string fileop::basename(std::string fn) {
    if (!is_url(fn)) {
        auto i = fn.find_last_of('/');
        auto i2 = fn.find_last_of('\\');
        i = (i == std::string::npos || (i2 != std::string::npos && i2 > i)) ? i2 : i;
        return i == std::string::npos ? fn : fn.substr(i + 1, fn.length() - i - 1);
    } else {
        auto iq = fn.find_first_of('?');
        auto iq2 = fn.find_first_of('#');
        iq = (iq == std::string::npos || (iq2 != std::string::npos && iq2 < iq)) ? iq2 : iq;
        auto i = fn.find_last_of('/', iq);
        auto i2 = fn.find_last_of('\\', iq);
        i = (i == std::string::npos || (i2 != std::string::npos && i2 > i)) ? i2 : i;
        if (i == std::string::npos && iq == std::string::npos) {
            return fn;
        } else if (i == std::string::npos) {
            return fn.substr(0, iq);
        } else if (iq == std::string::npos) {
            return fn.substr(i + 1, fn.length() - i - 1);
        } else {
            return fn.substr(i + 1, iq - i - 1);
        }
    }
}

bool fileop::parse_size(std::string size, size_t& fs, bool is_byte) {
    const std::regex REG(R"(^(\d+)([kmgtpezy])?(i)?(b)?$)", std::regex::icase);
    std::smatch m;
    if (!std::regex_search(size, m, REG)) {
        return false;
    }
    auto bs = m[1].str();
    size_t base;
    if (sscanf(bs.c_str(), "%zu", &base) != 1) {
        return false;
    }
    size_t pow = 0;
    if (m[2].matched) {
        auto sp = std::tolower(m[2].str()[0]);
        if (sp == 'k') pow = 1;
        else if (sp == 'm') pow = 2;
        else if (sp == 'g') pow = 3;
        else if (sp == 't') pow = 4;
        else if (sp == 'p') pow = 5;
        else if (sp == 'e') pow = 6;
        else if (sp == 'z') pow = 7;
        else if (sp == 'y') pow = 8;
    }
    if (m[3].matched) {
        pow = 1ull << (pow * 10);
    } else {
        size_t tmp = 1;
        while (pow > 0) {
            tmp *= 1000ull;
            pow--;
        }
        pow = tmp;
    }
    if (m[4].matched) {
        auto b = m[4].str();
        if (b == "b") {
            fs = is_byte ? base * pow / 8ull : base * pow;
        } else {
            fs = is_byte ? base * pow : base * pow * 8ull;
        }
    } else {
        fs = base * pow;
    }
    return true;
}
