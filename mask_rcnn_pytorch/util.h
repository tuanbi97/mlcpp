#pragma once

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

struct Point
{
    int32_t x{0};
    int32_t y{0};
};

struct BoundingBox
{
    Point top_left;
    int32_t width{0};
    int32_t height{0};
};

struct StrToFloat
{
    float operator()(std::string const &str) { return std::stof(str); }
};

// trim from start (in place)
static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace)))
                .base(),
            s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}