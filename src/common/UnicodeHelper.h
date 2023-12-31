//
// Created by gnilk on 09.09.23.
//

#ifndef GNILK_UNICODEHELPER_H
#define GNILK_UNICODEHELPER_H

#include <string>

namespace gnilk {
    class UnicodeHelper {
    public:
        static bool ConvertUTF8ToUTF32String(std::u32string &out, const std::string &src);
        static bool ConvertUTF8ToASCII(std::string &out, const std::string &src);
        static bool ConvertUTF32ToUTF8String(std::string &out, const std::u32string &src);
        static bool ConvertUTF32ToUTF8String(std::u8string &out, const std::u32string &src);
        static bool ConvertUTF32ToASCII(std::string &out, const std::u32string &src);


        static std::u32string utf8to32(const std::string &src);
        static std::string utf32to8(const std::u32string &src);
        static std::string utf8toascii(const std::string &src);
        static std::string utf8toascii(const char8_t *src);
        static std::string utf32toascii(const std::u32string &src);
    };
}


#endif //SDLAPP_UNICODEHELPER_H
