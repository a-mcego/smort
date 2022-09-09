#pragma once

//this removes the path from __FILE__ so my output doesn't get polluted
inline consteval int wrangle_path(const char* c)
{
    int last_slash=0;
    int n=0;
    while(c[n] != 0)
    {
        if (c[n] == '/' || c[n] == '\\')
            last_slash=n;
        ++n;
    }
    return last_slash+1;
}
#define smort_assert(x) ((x)?(void)0:(std::cerr << "Assert failed in " << (__FILE__+wrangle_path(__FILE__)) << ":" << __LINE__ << ": " << #x << std::endl, std::abort()))
