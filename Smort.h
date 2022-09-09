#pragma once

#include <stack>
using std::cout, std::endl;

#include "SmortAssert.h"
#include "StrongTypedef.h"

using datum = int;
MakeStrong(ClassID, datum);
MakeStrong(ClassValue, datum);

template<typename T>
struct Iter
{
    T val;

    bool operator!=(const Iter& rhs)
    {
        return val != rhs.val;
    }

    Iter& operator++()
    {
        ++val;
        return *this;
    }

    T operator*()
    {
        return val;
    }
};

struct ClassRange
{
    const ClassValue lowest, highest; //both *inclusive*
    const ClassID n_classes;

    ClassRange(ClassValue l_, ClassValue h_):lowest(l_),highest(h_),n_classes(h_.toT()-l_.toT()+1) { smort_assert(n_classes > ClassID(0)); }
    ClassRange(ClassID size):lowest(0),highest(ClassValue(size.toT()-1)),n_classes(size) {}

    ClassID ToClassID(const ClassValue& val) const
    {
        if(val >= lowest and val <= highest)
            return ClassID((val.toT() - lowest.toT()));
        return ClassID(-1);
    }

    int N() const { return n_classes.toT(); }

    Iter<ClassValue> begin() { return Iter<ClassValue>(ClassValue(lowest)); }
    Iter<ClassValue> end() { return Iter<ClassValue>(highest+ClassValue(1)); }
};

#include "Collectors.h"

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
    static int CountPossibleOnes(const vectorview<datum>& v, int N)
    {
        int ret=0;
        for(int i=0; i<N; ++i)
            ret += (v[i]!=0?1:0);
        return ret;
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

    void SelfCheck() const
    {
        smort_assert(defs.first.cell_ids.size() == defs.second.cell_ids.size());
    }
};


enum struct SOLVESTATE
{
    FAILURE,
    INDETERMINATE,
    SUCCESS,
};

SOLVESTATE Combine(SOLVESTATE a, SOLVESTATE b)
{
    if (a==SOLVESTATE::FAILURE || b==SOLVESTATE::FAILURE)
        return SOLVESTATE::FAILURE;
    if (a==SOLVESTATE::INDETERMINATE || b==SOLVESTATE::INDETERMINATE)
        return SOLVESTATE::INDETERMINATE;
    return SOLVESTATE::SUCCESS;
}

struct Constraint
{
    enum struct TYPE
    {
        WORLD,
        ALL_DIFFERENT,
        N
    } type;

    //int n_classes;

    ClassRange classrange;

    int n_cells;
    std::vector<datum> data;

    datum& operator()(int cell_n, ClassID class_id)
    {
        return data[cell_n*classrange.N()+class_id.toT()];
    }

    void Restrict(int cell_class_id)
    {
        int cell_id = cell_class_id/classrange.N();
        int class_id = cell_class_id%classrange.N();

        for(int class_i=0; class_i<classrange.N(); ++class_i)
        {
            data[cell_id*classrange.N()+class_i] = (class_i==class_id?1:0);
        }
    }

    SOLVESTATE GetSolveState()
    {
        SOLVESTATE ret = SOLVESTATE::SUCCESS;

        for(int i=0; i<n_cells; ++i)
        {
            int ones = Ops::CountPossibleOnes(vectorview<datum>(&data,i*classrange.N()), classrange.N());

            if (ones == 0)
                return SOLVESTATE::FAILURE;
            if (ones > 1)
                ret = SOLVESTATE::INDETERMINATE;
        }
        return ret;
    }

    //returns pair<cell_id, score>
    MinCollector<int,int> GetMostConstrainedUnsolvedCell()
    {
        //cells are y-axis
        //classes are x-axis
        std::vector<int> cell_sums(n_cells,0);
        std::vector<int> class_sums(classrange.N(),0);

        for(int cell=0; cell<n_cells; ++cell)
        {
            for(int cl=0; cl<classrange.N(); ++cl)
            {
                if (data[cell*classrange.N()+cl] != 0)
                {
                    cell_sums[cell] += 1;
                    class_sums[cl] += 1;
                }
            }
        }

        MinCollector<int,int> collector;
        for(int cell=0; cell<n_cells; ++cell)
        {
            for(int cl=0; cl<classrange.N(); ++cl)
            {
                int coord = cell*classrange.N()+cl;
                if (data[coord] == 0)
                    continue;
                int score = cell_sums[cell]+class_sums[cl];
                if (cell_sums[cell]+class_sums[cl] == 2)
                    continue;
                collector.insert(score, coord);
            }
        }
        return collector;
    }

    bool UpdateWORLD() { return false; }

    bool UpdateALL_DIFFERENT()
    {
        bool changed = false;
        const int Y = n_cells, X = classrange.N();

        for(int x=0; x<X; ++x)
        {
            //find naked single
            int sum=0, last_y=-1;
            for(int y=0; y<Y; ++y)
            {
                datum val = data[y*classrange.N()+x];
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
                    changed |= (data[last_y*classrange.N()+x2] != (x2==x));
                    data[last_y*classrange.N()+x2] = (x2==x);
                }

            }
        }
        //samma på svenska
        for(int y=0; y<Y; ++y)
        {
            int sum=0, last_x=-1;
            for(int x=0; x<X; ++x)
            {
                datum val = data[y*classrange.N()+x];
                sum += val;
                if (val)
                    last_x=x;
            }
            if (sum == 1)
            {
                smort_assert(last_x!=-1);
                for(int y2=0; y2<Y; ++y2)
                {
                    changed |= (data[y2*classrange.N()+last_x] != (y2==y));
                    data[y2*classrange.N()+last_x] = (y2==y);
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

    //Constraint(TYPE type_, int n_cells_, int n_classes_):type(type_), classrange(ClassValue(0),ClassValue(n_classes_)), n_cells(n_cells_)
    Constraint(TYPE type_, int n_cells_, ClassRange classrange_):type(type_), classrange(classrange_), n_cells(n_cells_)

    {
        data = std::vector<datum>(n_cells*classrange.N(), true);
    }

    std::vector<vectorview<datum>> View(const std::vector<size_t>& ids)
    {
        std::vector<vectorview<datum>> data_ptrs;
        for(size_t id: ids)
        {
            smort_assert(id < n_cells);
            data_ptrs.push_back(vectorview<datum>(&data,id*classrange.N()));
        }
        return data_ptrs;
    }

    vectorview<datum> View(const size_t id=0)
    {
        return vectorview<datum>(&data,id*classrange.N());
    }
};


struct Hypothesis
{
    int constraint_id=-1, cell_class_id=-1;
};


struct Smort
{
    std::stack<Hypothesis> hypotheses;
    struct State
    {
        int n_classes;//TODO: each constraint can have different classes, and relations have the mapping
        std::vector<Constraint> constraints;
        std::vector<Relation> relations;
    };

    std::stack<State> states;

    Smort()
    {
        states.push(State());
    }

    State& GetState()
    {
        return states.top();
    }

    SOLVESTATE GetSolveState()
    {
        if(states.empty())
            return SOLVESTATE::FAILURE;

        SOLVESTATE ret = SOLVESTATE::SUCCESS;

        for(Constraint& c: GetState().constraints)
            ret = Combine(ret, c.GetSolveState());
        return ret;
    }

    int hypotheses_made = 0;

    SOLVESTATE PropagateConstraints()
    {

        while (true)
        {
            bool changed = false;

            //propagate between constraints
            for(Relation& r: GetState().relations)
            {
                Constraint& c1 = GetState().constraints[r.defs.first.constraint_id];
                Constraint& c2 = GetState().constraints[r.defs.second.constraint_id];

                ClassRange& cr1 = c1.classrange;
                ClassRange& cr2 = c2.classrange;

                std::vector<vectorview<datum>> v1 = c1.View(r.defs.first.cell_ids);
                std::vector<vectorview<datum>> v2 = c2.View(r.defs.second.cell_ids);

                smort_assert(v1.size() == v2.size());

                for(int v_i=0; v_i<v1.size(); ++v_i)
                {
                    //cout << "CV1 loop" << endl;
                    for(ClassValue cv1: cr1)
                    {
                        ClassID ci1 = cr1.ToClassID(cv1);
                        ClassID ci2 = cr2.ToClassID(cv1); //cv1 intentional!

                        //cout << cv1 << " " << ci1 << " " << ci2 << endl;

                        auto& val1 = v1[v_i][ci1.toT()];
                        if (ci2 == ClassID(-1)) //class isnt found on c2
                        {
                            changed |= (val1 != 0);
                            val1 = 0;
                        }
                        else
                        {
                            auto& val2 = v2[v_i][ci2.toT()];
                            bool different = (val1 != val2);
                            changed = (changed || different);
                            if (different)
                            {
                                v1[v_i][ci1.toT()] = 0;
                                v2[v_i][ci2.toT()] = 0;
                            }
                        }
                    }
                    //cout << "CV2 loop" << endl;
                    for(ClassValue cv2: cr2)
                    {
                        ClassID ci1 = cr1.ToClassID(cv2);
                        ClassID ci2 = cr2.ToClassID(cv2); //cv2 intentional!

                        //cout << cv2 << " " << ci1 << " " << ci2 << endl;

                        auto& val2 = v2[v_i][ci2.toT()];
                        if (ci1 == ClassID(-1)) //class isnt found on c2
                        {
                            changed |= (val2 != 0);
                            val2 = 0;
                        }
                        else
                        {
                            auto& val1 = v1[v_i][ci1.toT()];
                            bool different = (val2 != val1);
                            changed |= different;
                            if (different)
                            {
                                val2 = 0;
                                val1 = 0;
                            }
                        }
                    }
                    //cout << "End loops" << endl;
                }
            }

            //do constraint-internal updates
            for(Constraint& c: GetState().constraints)
                changed |= c.Update();
            if (!changed)
                break;
        }

        return GetSolveState();
    }

    void Solve()
    {
        //std::cout << "Solve called!" << std::endl;
        smort_assert(GetState().constraints.size() > 0);
        smort_assert(GetState().relations.size() > 0);
        for(const auto& relation: GetState().relations)
            relation.SelfCheck();
        while(true)
        {
            //cout << "--------------------------------------------------" << endl;
            //cout << states.size() << " states." << endl;
            //std::cout << "Propagating constraints..." << std::endl;
            SOLVESTATE st = PropagateConstraints();
            //std::cout << "Result: " << int(st) << " (0=bad, 1=indeterminate, 2=good)" << std::endl;
            if (st == SOLVESTATE::SUCCESS)
                return;
            if (st == SOLVESTATE::FAILURE)
            {
                if (hypotheses.empty())
                    return;
                //mark the opposite of the hypothesis
                states.pop();
                if (states.empty())
                    return;
                Hypothesis h = hypotheses.top();
                hypotheses.pop();
                auto& c = GetState().constraints[h.constraint_id];
                c.data[h.cell_class_id] = 0;
            }

            if (st == SOLVESTATE::INDETERMINATE)
            {
                //push new state on stack
                states.push(states.top());

                //make hypothesis
                MinCollector<int,Hypothesis> hypo_collector;

                for(int c_id=0; c_id<GetState().constraints.size(); ++c_id)
                {
                    Constraint& c = GetState().constraints[c_id];
                    Hypothesis h;
                    h.constraint_id = c_id;

                    MinCollector<int,int> p = c.GetMostConstrainedUnsolvedCell();
                    if (!p.is_initialized())
                        continue;

                    h.cell_class_id = p.data().second;
                    hypo_collector.insert(p.data().first,h);
                }

                Hypothesis d = hypo_collector.data().second;
                //std::cout << "Hypothesis found: " << d.constraint_id << ":" << d.cell_class_id << " with score " << hypo_collector.data().first << std::endl;

                Constraint& c = GetState().constraints[d.constraint_id];
                c.Restrict(d.cell_class_id);
                hypotheses.push(d);
                ++hypotheses_made;
            }


            if (states.empty())
            {
                return;
            }
        }
    }
};
