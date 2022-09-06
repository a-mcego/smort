#pragma once


template<typename T>
T FromString(std::string key)
{
    if constexpr(std::is_same_v<T,std::string>)
        return key;

    std::stringstream sstr(key);
	T ret;
	sstr >> ret;
	return ret;
}
template<typename T>
std::string ToString(T key)
{
    if constexpr(std::is_same_v<T,std::string>)
        return key;

    std::stringstream sstr;
	sstr << key;
	return sstr.str();
}

inline const std::vector<std::string> Explode(const std::string& s, const char& c)
{
	std::string buff{""};
	std::vector<std::string> v;

	for(auto n:s)
	{
		if(n != c) buff+=n; else
		if(n == c && buff != "") { v.push_back(buff); buff = ""; }
	}
	if(buff != "") v.push_back(buff);

	return v;
}

template<typename... T>
auto MakeVector(const T&... t)
{
    using firsttype_t = std::tuple_element_t<0, std::tuple<T...>>;
    static_assert((std::is_same_v<firsttype_t,T> && ...));
    return std::vector<firsttype_t>{t...};
}

template<typename... T>
auto MakeArray(const T&... t)
{
    using firsttype_t = std::tuple_element_t<0, std::tuple<T...>>;
    static_assert((std::is_same_v<firsttype_t,T> && ...));
    return std::array<firsttype_t,sizeof...(T)>{t...};
}

#include <chrono>
const long long int starttime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
inline long long int cloque()
{ return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - starttime; }
