#pragma once



template<typename K, typename V, typename Compare = std::less<K>>
struct MaxCollector
{
private:
    std::pair<K,V> internal_data;
    bool initialized=false;
public:

    void insert(const K& k, const V& v)
    {

        if (!initialized)
        {
            //std::cout << "Initializing with key " << k << std::endl;
            internal_data.first = k, internal_data.second = v, initialized=true;
        }
        else if (Compare()(internal_data.first, k))
        {
            //std::cout << "Replacing key " << internal_data.first << " with key " << k << std::endl;
            internal_data.first = k, internal_data.second = v;
        }
    }

    void insert(const std::pair<K,V>& p) { insert(p.first, p.second); }
    std::pair<K,V> data() { return internal_data; }
    bool is_initialized() { return initialized; }
};

template<typename T, typename U, typename Compare = std::greater<T>>
using MinCollector = MaxCollector<T,U,std::greater<T>>;

