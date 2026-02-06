#pragma once

#include <string>
#include <vector>
#include <iconv.h>
#include <cstring>
#include <iostream>

namespace QuantLabs {
namespace utils {

inline std::string gbk_to_utf8(const std::string& gbk_str) {
    if (gbk_str.empty()) return "";

    iconv_t cd = iconv_open("UTF-8", "GB18030");
    if (cd == (iconv_t)-1) {
        std::cerr << "[Encoding] iconv_open failed" << std::endl;
        return gbk_str;
    }

    size_t in_len = gbk_str.size();
    size_t out_len = in_len * 2 + 1; // max UTF8 expansion
    std::vector<char> out_buf(out_len);

    char* in_ptr = const_cast<char*>(gbk_str.c_str());
    char* out_ptr = out_buf.data();
    size_t in_bytes_left = in_len;
    size_t out_bytes_left = out_len;

    // Ignore invalid sequences to avoid stopping
    // iconv implementation might vary, but usually we just process what we can
    size_t res = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
    
    if (res == (size_t)-1) {
         // If error, might be partly converted. We could still use it or fallback.
         // For simplistic robustness, we just use what we converted so far if possible, or return original if totally failed
         // std::cerr << "[Encoding] iconv failed: " << strerror(errno) << std::endl;
    }

    iconv_close(cd);
    return std::string(out_buf.data(), out_buf.size() - out_bytes_left);
}

}
}
