#include "wchar_util.h"
#include "list_pointer.h"
#include <malloc.h>

#if _WIN32
#include <Windows.h>
#endif

bool stringToChar(std::string input, char*& output) {
	auto sz = input.size();
	auto s = (char*)malloc(sz + 1);
	if (!s) {
		return false;
	}
	memcpy(s, input.c_str(), sz);
	s[sz] = 0;
	output = s;
	return true;
}

void freeChar(char* input) {
	if (input) free(input);
}

#if _WIN32
unsigned long wchar_util::getMultiByteToWideCharOptions(const unsigned long ori_options, unsigned int cp) {
	if (cp == CP_ACP) cp = GetACP();
	if (cp == CP_OEMCP) cp = GetOEMCP();
	switch (cp)
	{
	case 50220:
	case 50221:
	case 50222:
	case 50225:
	case 50227:
	case 50229:
	case CP_UTF7:
	case 42:
		return 0;
	default:
		break;
	}
	if (cp >= 57002 && cp <= 57011) return 0;
	if (cp == CP_UTF8 || cp == 54936) {
		return MB_ERR_INVALID_CHARS & ori_options;
	}
	return ori_options;
}

unsigned long wchar_util::getWideCharToMultiByteOptions(const unsigned long ori_options, unsigned int cp) {
	if (cp == CP_ACP) cp = GetACP();
	if (cp == CP_OEMCP) cp = GetOEMCP();
	switch (cp)
	{
	case 50220:
	case 50221:
	case 50222:
	case 50225:
	case 50227:
	case 50229:
	case CP_UTF7:
	case 42:
		return 0;
	default:
		break;
	}
	if (cp >= 57002 && cp <= 57011) return 0;
	if (cp == CP_UTF8 || cp == 54936) {
		return WC_ERR_INVALID_CHARS & ori_options;
	}
	return ori_options;
}

bool wchar_util::str_to_wstr(std::wstring& out, std::string inp, unsigned int cp) {
    DWORD opt = getMultiByteToWideCharOptions(MB_ERR_INVALID_CHARS, cp);
    int wlen = MultiByteToWideChar(cp, opt, inp.c_str(), inp.length(), nullptr, 0);
    if (!wlen) {
        return false;
    }
    wchar_t* wstr = (wchar_t*) malloc(wlen * sizeof(wchar_t));
    if (wstr == nullptr) {
        return false;
    }
    if (!MultiByteToWideChar(cp, opt, inp.c_str(), inp.length(), wstr, wlen)) {
        free(wstr);
        return false;
    }
    out = std::wstring(wstr, wlen);
    free(wstr);
    return true;
}

bool wchar_util::wstr_to_str(std::string& out, std::wstring inp, unsigned int cp) {
    DWORD opt = getWideCharToMultiByteOptions(WC_ERR_INVALID_CHARS, cp);
    int len = WideCharToMultiByte(cp, opt, inp.c_str(), inp.length(), nullptr, 0, nullptr, nullptr);
    if (!len) {
        return false;
    }
    char* str = (char*) malloc(len);
    if (str == nullptr) {
        return false;
    }
    if (!WideCharToMultiByte(cp, opt, inp.c_str(), inp.length(), str, len, nullptr, nullptr)) {
        free(str);
        return false;
    }
    out = std::string(str, len);
    free(str);
    return true;
}

/**
 * @brief Convert wstring version argv to UTF-8 encoding argv
 * @param ArgvW Source
 * @param argc The count of parameters
 * @param argv dest (Need free memory by calling freeArgv)
 * @return true if OK
*/
bool ArgvWToArgv(const LPWSTR* ArgvW, int argc, char**& argv) {
	if (!ArgvW) return false;
	std::list<std::string> li;
	for (int i = 0; i < argc; i++) {
		if (!ArgvW[i]) return false;
		std::wstring s = ArgvW[i];
		std::string r;
		if (!wchar_util::wstr_to_str(r, s, CP_UTF8)) return false;
		li.push_back(r);
	}
	char** t;
	if (!listToPointer<std::string, char*>(li, t, &stringToChar, &freeChar)) {
		return false;
	}
	argv = t;
	return true;
}

bool wchar_util::getArgv(char**& argv, int& argc) {
	auto cm = GetCommandLineW();
	int argcw = 0;
	auto argvw = CommandLineToArgvW(cm, &argcw);
	if (!argvw) return false;
	char** t;
	if (!ArgvWToArgv(argvw, argcw, t)) {
		LocalFree(argvw);
		return false;
	}
	LocalFree(argvw);
	argc = argcw;
	argv = t;
	return true;
}

void wchar_util::freeArgv(char** argv, int argc) {
	freePointerList<char*>(argv, argc, nullptr);
}
#endif
