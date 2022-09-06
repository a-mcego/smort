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

using datum = int;

template<typename T>
struct vectorview
{
    std::vector<T>*const v = nullptr;
    int start;

    vectorview(std::vector<T>* v_, int start_):v(v_),start(start_)
    {
        smort_assert(v != nullptr);
        smort_assert(v->size() > start);
    }

    T& operator[](int n)
    {
        return v->at(start+n);
    }
    const T& operator[](int n) const
    {
        return v->at(start+n);
    }

    long long int size()
    {
        return v->size();
    }
};

struct Ops
{
    template<bool value>
    static void SetAll(vectorview<datum>& b, int N)
    {
        for(int i=0; i<N; ++i)
            b[i] = value;
    }

    static bool And(vectorview<datum>& b1, vectorview<datum>& b2, int N)
    {
        bool changed = false;
        for(int i=0; i<N; ++i)
        {
            if (b1[i] != b2[i])
                changed = true;
            b1[i] &= b2[i];
            b2[i] = b1[i];
        }
        return changed;
    }

    static bool And(std::vector<vectorview<datum>>& v1, std::vector<vectorview<datum>>& v2, int N)
    {
        bool changed = false;
        smort_assert(v1.size() == v2.size());
        smort_assert(v1.size() > 0);
        for(int i=0; i<int(v1.size()); ++i)
            changed |= And(v1.at(i),v2.at(i),N);
        return changed;
    }


    template<bool value>
    static void SetAll(const std::vector<vectorview<datum> >& v, int N)
    {
        for(vectorview<datum>& b: v)
            SetAll<value>(b,N);
    }

    static void SetOneTrue(vectorview<datum>& b, int pos, int N)
    {
        smort_assert(pos < N);
        SetAll<false>(b,N);
        b[pos] = true;
    }
};

struct Relation
{
    struct Def
    {
        size_t constraint_id;
        std::vector<size_t> cell_ids;
    };
    std::pair<Def,Def> defs;
};

struct Constraint
{
    enum struct TYPE
    {
        WORLD,
        ALL_DIFFERENT,
        N
    } type;

    bool UpdateWORLD() { return false; }

    bool UpdateALL_DIFFERENT()
    {
        bool changed = false;
        const int Y = n_cells, X = n_classes;

        for(int x=0; x<X; ++x)
        {
            //find naked single
            int sum=0, last_y=-1;
            for(int y=0; y<Y; ++y)
            {
                datum val = data[y*n_classes+x];
                sum += val;
                if (val)
                    last_y=y;
            }
            //if found, set the corresponding other thing to onehot
            if (sum == 1)
            {
                smort_assert(last_y!=-1);
                for(int x2=0; x2<X; ++x2)
                {
                    changed |= (data[last_y*n_classes+x2] != (x2==x));
                    data[last_y*n_classes+x2] = (x2==x);
                }

            }
        }
        //samma på svenska
        for(int y=0; y<Y; ++y)
        {
            int sum=0, last_x=-1;
            for(int x=0; x<X; ++x)
            {
                datum val = data[y*n_classes+x];
                sum += val;
                if (val)
                    last_x=x;
            }
            if (sum == 1)
            {
                smort_assert(last_x!=-1);
                for(int y2=0; y2<Y; ++y2)
                {
                    changed |= (data[y2*n_classes+last_x] != (y2==y));
                    data[y2*n_classes+last_x] = (y2==y);
                }

            }
        }
        return changed;
    }

    static constexpr bool(Constraint::*const update_methods[int(TYPE::N)])() =
    {
        &Constraint::UpdateWORLD,
        &Constraint::UpdateALL_DIFFERENT,
    };

    bool Update() { return (this->*update_methods[int(type)])(); }

    int n_classes;
    int n_cells;
    std::vector<datum> data;

    Constraint(TYPE type_, int n_cells_, int n_classes_):type(type_), n_classes(n_classes_), n_cells(n_cells_)
    {
        data = std::vector<datum>(n_cells*n_classes, true);
    }

    std::vector<vectorview<datum>> View(const std::vector<size_t>& ids)
    {
        std::vector<vectorview<datum>> data_ptrs;
        for(size_t id: ids)
        {
            smort_assert(id < n_cells);
            data_ptrs.push_back(vectorview<datum>(&data,id*n_classes));
        }
        return data_ptrs;
    }
};

struct Smort
{
    int n_classes;
    std::vector<Constraint> constraints;
    std::vector<Relation> relations;

    void Solve()
    {
        while (true)
        {
            bool changed = false;

            //propagate between constraints
            for(Relation& r: relations)
            {
                auto lhs = constraints[r.defs.first.constraint_id].View(r.defs.first.cell_ids);
                auto rhs = constraints[r.defs.second.constraint_id].View(r.defs.second.cell_ids);
                changed |= Ops::And(lhs, rhs, n_classes);
            }

            //do constraint-internal updates
            for(Constraint& c: constraints)
                changed |= c.Update();
            if (!changed)
                break;
        }
    }
};
