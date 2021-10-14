#ifndef _UTIL_WCHAR_UTIL_H
#define _UTIL_WCHAR_UTIL_H
#include<string>

namespace wchar_util {
#if _WIN32
    /**
     * @brief Get correct options for MultiByteToWideChar
     * @param ori_options Origin options
     * @param cp Code Page
     * @return Result options
    */
    unsigned long getMultiByteToWideCharOptions(const unsigned long ori_options, unsigned int cp);
    /**
     * @brief Get correct options for WideCharToMultiByte
     * @param ori_options Origin options
     * @param cp Code Page
     * @return Result options
    */
    unsigned long getWideCharToMultiByteOptions(const unsigned long ori_options, unsigned int cp);
    /**
     * @brief Convert string to wstring
     * @param out Output string
     * @param inp Input string
     * @param cp Code page of input string
     * @returns true if successed
    */
    bool str_to_wstr(std::wstring& out, std::string inp, unsigned int cp);
    /**
     * @brief Convert wstring to string
     * @param out Output string
     * @param inp Input string
     * @param cp Code page of output string
     * @returns true if successed
    */
    bool wstr_to_str(std::string& out, std::wstring inp, unsigned int cp);
    /**
     * @brief Get unicode version argv by using win api
     * @param argv Result argv (need free memory by calling freeArgv.
     * @param argc argc
     * @return true if OK
    */
    bool getArgv(char**& argv, int& argc);
    void freeArgv(char** argv, int argc);
#endif
}
#endif
