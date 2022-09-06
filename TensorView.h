#pragma once

template<typename T, int RANK>
struct TensorView
{
    const std::vector<T>* v = nullptr; //sorry jason
    std::array<size_t,RANK> dims;
    std::array<size_t,RANK> mults;
    TensorView(const std::vector<T>* v_, const std::array<size_t,RANK>& dims_):v(v_),dims(dims_)
    {
        for(int i=0; i<RANK; ++i)
            mults[i] = 1;
        for(int i=0; i<RANK; ++i)
            for(int j=i+1; j<RANK; ++j)
                mults[i] *= dims[j];
        smort_assert(mults[0]*dims[0] == v_->size());
    }

    template<typename... U>
    T& operator()(const U&... index)
    {
        static_assert(sizeof...(U) == RANK, "wrong amount of dims given to operator()");
        int i=0, ii=0;
        ((i += index*mults[ii], ++ii),...);
        return v->at(i);
    }
};

