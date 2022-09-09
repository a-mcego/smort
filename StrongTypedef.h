#pragma once

namespace StrongTypedef
{
    inline int t(int x) { return x; } //only for throwing errors from name() which is a consteval function

    consteval long long int name(const char* c)
    {
        long long int ret=0;
        for(int i=0; (*c)!=0; ++i)
        {
            char value = *c;
            if (value >= 'A' and value <= 'Z')
                value += 32;
            if (value < 'a' or value > 'z')
                t(0);
            if (i<13)
                ret = (ret*26)+int(value);
            else
                t(1);
            ++c;
        }
        return ret;
    }
}
template<typename T, long long int ID_>
struct Strong
{
private:
    T t;
public:
    static constexpr long long int ID = ID_;

    T toT() const { return t; }

    explicit Strong(const T& val) { t = val; }

    auto operator<=>(const Strong<T,ID_>&) const = default;

    //reimplement all the operators! \o/

    Strong<T,ID_>& operator++(){ ++t; return *this; }

#define implement_op(OP) \
    Strong<T,ID_>& operator OP ## = (const Strong<T,ID_>& rhs) { t OP ## = rhs.t; return *this; }\
    Strong<T,ID_> operator OP (const Strong<T,ID_>& rhs) const { Strong<T,ID_> ret = *this; ret OP ## = rhs; return ret; }

    implement_op(+);
    implement_op(-);
    implement_op(*);
    implement_op(/);
    implement_op(<<);
    implement_op(>>);
#undef implement_op
};

template<typename T, long long int ID_>
std::ostream& operator<<(std::ostream& o, const Strong<T,ID_>& t)
{
    o << t.toT();
    return o;
}

#define MakeStrong(classname,t) using classname = Strong<int,StrongTypedef::name(#classname)>;
