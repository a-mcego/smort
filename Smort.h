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
    ClassValue lowest=ClassValue(0), highest=ClassValue(0); //both *inclusive*
    ClassID n_classes=ClassID(-1);

    ClassRange() = default;
    ClassRange( const ClassRange & ) = default;
    ClassRange( ClassRange && ) = default;
    ClassRange& operator=( const ClassRange & ) = default;

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

    bool in_range(ClassValue val)
    {
        return lowest <= val and val <= highest;
    }
};

#include "Collectors.h"

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

#include "TensorView.h"

struct Constraint
{
    enum struct TYPE
    {
        WORLD,
        ALL_DIFFERENT,
        LESS_THAN,
        N
    } type;

    struct Cell
    {
        ClassRange classrange;
        std::vector<datum> data;
        Cell(const ClassRange& cr_):classrange(cr_)
        {
            Assign<1>();
        }

        Cell(const Cell& p1):classrange(p1.classrange),data(p1.data)
        {
        }
        template<datum VAL>
        void Assign()
        {
            data.assign(classrange.N(), VAL);
        }

        template<datum VAL>
        std::optional<ClassValue> lowest_value()
        {
            std::optional<ClassValue> ret;
            for(ClassValue cv: classrange)
            {
                if(get(cv) == VAL)
                {
                    ret = cv;
                    break;
                }
            }
            return ret;
        }

        template<datum VAL>
        std::optional<ClassValue> highest_value()
        {
            std::optional<ClassValue> ret;
            for(ClassValue cv: classrange)
            {
                if(get(cv) == VAL)
                    ret = cv;
            }
            return ret;
        }

        void LimitLowerBound(ClassValue cv)
        {
            if (classrange.lowest > cv)
                return;
            smort_assert(classrange.highest >= cv);
            Cell newcell{ClassRange{cv,classrange.highest}};
            for(ClassValue newcv: newcell.classrange)
                newcell.set(newcv,get(newcv));
            classrange.lowest = cv;
            data = newcell.data;
        }

        void LimitUpperBound(ClassValue cv)
        {
            if (classrange.highest < cv)
                return;
            smort_assert(classrange.lowest <= cv);
            Cell newcell{ClassRange{classrange.lowest,cv}};
            for(ClassValue newcv: newcell.classrange)
                newcell.set(newcv,get(newcv));
            classrange.highest = cv;
            data = newcell.data;
        }

        datum get(ClassValue cv)
        {
            if (!classrange.in_range(cv))
                return 0;
            return data[classrange.ToClassID(cv).toT()];
        }
        void set(ClassValue cv, datum d)
        {
            if (!classrange.in_range(cv))
                return;
            data[classrange.ToClassID(cv).toT()] = d;
        }

        int CountOnes()
        {
            int ones=0;
            for(datum d: data)
                if (d==1)
                    ++ones;
            return ones;
        }

        void SetToOneHot(ClassValue cv)
        {
            smort_assert(classrange.in_range(cv));
            data.assign(classrange.N(), 0);
            data[classrange.ToClassID(cv).toT()] = 1;
        }

    };
    std::vector<Cell> cells;

    datum& operator()(int cell_n, ClassID class_id)
    {
        return cells[cell_n].data[class_id.toT()];
    }

    SOLVESTATE GetSolveState()
    {
        SOLVESTATE ret = SOLVESTATE::SUCCESS;

        for(int i=0; i<cells.size(); ++i)
        {
            int ones = cells[i].CountOnes();

            if (ones == 0)
                return SOLVESTATE::FAILURE;
            if (ones > 1)
                ret = SOLVESTATE::INDETERMINATE;
        }
        return ret;
    }

    //returns pair<cell_id, score>
    MinCollector<int,std::pair<int,ClassValue>> GetMostConstrainedUnsolvedCell()
    {
        MinCollector<int,std::pair<int,ClassValue>> collector;
        if (type != TYPE::ALL_DIFFERENT)
            return collector;

        //cells are y-axis
        //classes are x-axis
        std::vector<int> cell_sums(cells.size(),0);
        //std::vector<int> class_sums(classrange.N(),0);
        std::map<ClassValue,int> class_sums;

        for(int cell=0; cell<cells.size(); ++cell)
        {
            //for(int cl=0; cl<cells[cell].classrange.N(); ++cl)
            for(ClassValue cv: cells[cell].classrange)
            {
                if (cells[cell].get(cv) != 0)
                {
                    cell_sums[cell] += 1;
                    class_sums[cv] += 1;
                }
            }
        }

        for(int cell=0; cell<cells.size(); ++cell)
        {
            //for(int cl=0; cl<classrange.N(); ++cl)
            for(ClassValue cv: cells[cell].classrange)
            {
                /*int coord = cell*classrange.N()+cl;
                if (data[coord] == 0)
                    continue;*/
                if (cells[cell].get(cv) == 0)
                    continue;
                int score = cell_sums[cell]+class_sums[cv];
                if (cell_sums[cell]+class_sums[cv] == 2)
                    continue;
                collector.insert(score, std::make_pair(cell,cv));
            }
        }
        return collector;
    }

    bool UpdateWORLD() { return false; }

    /*
    bool UpdateALL_DIFFERENT()
    {
        bool changed = false;
        const int Y = n_cells, X = classrange.N();

        TensorView<datum, 2> tv {&data, {Y,X}};

        return changed;
    }

    /*/
    bool UpdateALL_DIFFERENT()
    {
        std::set<ClassValue> cvs;
        for(Cell& c: cells)
        {
            for(ClassValue cv: c.classrange)
                cvs.insert(cv);
        }


        bool changed = false;
        const int Y = cells.size();

        for(ClassValue cv: cvs)
        {
            //find naked single
            int sum=0, last_y=-1; bool y_set=false;
            for(int y=0; y<Y; ++y)
            {
                datum val = cells[y].get(cv);
                sum += val;
                if (val)
                    last_y=y, y_set=true;
            }
            //if found, set the corresponding other thing to onehot
            if (sum == 1)
            {
                smort_assert(y_set);
                //for(int x2=0; x2<X; ++x2)
                for(ClassValue cv2: cvs)
                {
                    changed |= (cells[last_y].get(cv2) != (cv2==cv));
                    cells[last_y].set(cv2, (cv2==cv));
                }
            }
        }

        for(int y=0; y<Y; ++y)
        {
            //find naked single
            int sum=0; ClassValue last_cv{-1}; bool cv_set=false;
            for(ClassValue cv: cvs)
            {
                datum val = cells[y].get(cv);
                sum += val;
                if (val)
                    last_cv=cv, cv_set=true;
            }
            //if found, set the corresponding other thing to onehot
            if (sum == 1)
            {
                smort_assert(cv_set);
                //for(int x2=0; x2<X; ++x2)
                for(int y2=0; y2<Y; ++y2)
                {
                    changed |= (cells[y2].get(last_cv) != (y2==y));
                    cells[y2].set(last_cv, (y2==y));
                }
            }
        }
        return changed;
    }//*/

    bool UpdateLESS_THAN()
    {
        //cell[i] < cell[i+1] for all valid i
        for(int c_id=0; c_id<cells.size(); ++c_id)
        {
            Cell& c = cells[c_id];
            auto low_opt = c.lowest_value<1>();
            if (low_opt)
            {
                auto low = low_opt.value();
                for(int c_right_id=c_id+1; c_right_id<cells.size(); ++c_right_id)
                {
                    Cell& c2 = cells[c_right_id];
                    c2.LimitLowerBound(low+ClassValue(1));
                }
            }

            auto high_opt = c.highest_value<1>();
            if (high_opt)
            {
                auto high = high_opt.value();
                for(int c_left_id=0; c_left_id<c_id; ++c_left_id)
                {
                    Cell& c2 = cells[c_left_id];
                    c2.LimitUpperBound(high-ClassValue(1));
                }

            }
        }
        return false;
    }

    static constexpr bool(Constraint::*const update_methods[int(TYPE::N)])() =
    {
        &Constraint::UpdateWORLD,
        &Constraint::UpdateALL_DIFFERENT,
        &Constraint::UpdateLESS_THAN,
    };

    bool Update() { return (this->*update_methods[int(type)])(); }

    Constraint(TYPE type_, int n_cells, ClassRange classrange_):type(type_)
    {
        for(int i=0; i<n_cells; ++i)
            cells.push_back(Cell(classrange_));
    }
};


struct Hypothesis
{
    int constraint_id=-1;
    int cell_id=-1;
    ClassValue classvalue;
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
                for(int i=0; i<r.defs.first.cell_ids.size(); ++i)
                {
                    Constraint::Cell& cell1 = c1.cells[r.defs.first.cell_ids[i]];
                    Constraint::Cell& cell2 = c2.cells[r.defs.second.cell_ids[i]];

                    for(ClassValue cv1: cell1.classrange)
                    {
                        datum val1 = cell1.get(cv1);
                        datum val2 = cell2.get(cv1);
                        bool different = (val1 != val2);
                        changed |= different;
                        if (different)
                        {
                            cell1.set(cv1,0);
                            cell2.set(cv1,0);
                        }
                    }

                    for(ClassValue cv2: cell2.classrange)
                    {
                        datum val1 = cell1.get(cv2);
                        datum val2 = cell2.get(cv2);
                        bool different = (val1 != val2);
                        changed |= different;
                        if (different)
                        {
                            cell1.set(cv2,0);
                            cell2.set(cv2,0);
                        }
                    }
                }
            }

            //do constraint-internal updates
            int c_id=0;
            for(Constraint& c: GetState().constraints)
            {
                ++c_id;
                changed |= c.Update();
            }
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
                c.cells[h.cell_id].set(h.classvalue,0);
            }

            if (st == SOLVESTATE::INDETERMINATE)
            {
                //push new state on stack
                states.push(states.top());

                //make hypothesis
                //TODO: make hypothesis generation general
                //currently it only works for ALL_DIFFERENT constraints
                //but it could work like this: count how much the
                //possibilities change by assuming hypothesis and
                //choose the hypothesis that causes the lowest nonzero amount of change.
                MinCollector<int,Hypothesis> hypo_collector;

                for(int c_id=0; c_id<GetState().constraints.size(); ++c_id)
                {
                    Constraint& c = GetState().constraints[c_id];
                    Hypothesis h;
                    h.constraint_id = c_id;

                    MinCollector<int,std::pair<int,ClassValue>> p = c.GetMostConstrainedUnsolvedCell();
                    if (!p.is_initialized())
                        continue;

                    h.cell_id = p.data().second.first;
                    h.classvalue = p.data().second.second;
                    hypo_collector.insert(p.data().first,h);
                }

                Hypothesis d = hypo_collector.data().second;
                //std::cout << "Hypothesis found: constraint=" << d.constraint_id << ":cell=" << d.cell_class_id << " with score " << hypo_collector.data().first << std::endl;

                Constraint& c = GetState().constraints[d.constraint_id];
                c.cells[d.cell_id].SetToOneHot(d.classvalue);
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
