#include "Global.h"
#include "Util.h"
#include "Smort.h"

struct Sudoku
{
    int size;
    std::vector<int> clues;
    Smort smort;

    ClassRange clue_range;

    void MakeConstraints()
    {
        smort.GetState().n_classes = size;
        //constraint #0 is the world
        smort.GetState().constraints.push_back(Constraint{Constraint::TYPE::WORLD, size*size, clue_range});
        for(int Y=size; Y>=1; --Y)
        {
            int X = size/Y;
            if (X*Y != size)
                continue;
            for(int starty=0; starty<size; starty+=Y)
            {
                for(int startx=0; startx<size; startx+=X)
                {
                    Relation::Def world, area;

                    world.constraint_id = 0;
                    area.constraint_id = smort.GetState().constraints.size();

                    for(int dy=starty; dy<starty+Y; ++dy)
                    {
                        for(int dx=startx; dx<startx+X; ++dx)
                        {
                            world.cell_ids.push_back(dy*size+dx);
                            area.cell_ids.push_back(area.cell_ids.size());
                        }
                    }
                    Constraint c{Constraint::TYPE::ALL_DIFFERENT, size, clue_range};
                    smort.GetState().constraints.push_back(c);
                    smort.GetState().relations.push_back(Relation{std::make_pair(world, area)});
                }
            }
        }
    }

    std::string Solve()
    {
        //add the clues as additional constraints
        smort_assert(clues.size() == size*size);
        for(int i=0; i<size*size; ++i)
        {
            if (clues[i] == -1)
                continue;
            //constraint #0 is the world
            Constraint constraint{Constraint::TYPE::WORLD, 1, ClassRange(ClassValue(clues[i]),ClassValue(clues[i]))};

            smort.GetState().constraints.push_back(constraint);

            Relation::Def world_def = Relation::Def{0,{i}};
            Relation::Def clue_def = Relation::Def{smort.GetState().constraints.size()-1, {0}};

            smort.GetState().relations.push_back(Relation{std::make_pair(world_def,clue_def)});
        }

        //and now solve
        smort.Solve();

        //read the solution into the string
        //if a row is not one-hot, it writes nothing, leaving the cell as 0
        //which indicates that it wasnt fully solved
        std::string solution = std::string(size*size, '0');
        {
            for(int i=0; i<size; ++i)
            {
                for(int j=0; j<size; ++j)
                {
                    int sum=0,last=-1;
                    ClassRange& cr = smort.GetState().constraints[0].cells[i*9+j].classrange;
                    for(ClassValue cv: cr)
                    //for(int k=0; k<size; ++k)
                    {
                        datum n = smort.GetState().constraints[0].cells[i*9+j].get(cv);
                        if (n)
                            sum+=1, last=cr.ToClassID(cv).toT();
                    }
                    if (sum==1)
                        solution[i*9+j] = '1'+last;
                }
            }
        }
        return solution;
    }

    Sudoku(const std::string& str, int size_):size(size_),clue_range(ClassValue(1),ClassValue(size))
    {
        smort_assert(size*size == str.length());
        MakeConstraints();
        for(const char& c: str)
        {
            if (c == '0')
                clues.push_back(-1);
            else
                clues.push_back(c-'0');
        }
    }
};

void PrintSudoku(std::string s)
{
    for(int y=0; y<9; ++y)
    {
        for(int x=0; x<9; ++x)
        {
            std::cout << (s[y*9+x]=='0'?'.':s[y*9+x]);
            if (x==2 || x==5)
                std::cout << " ";
        }
        if (y==2 || y==5)
            std::cout << std::endl;
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

int numzeros(std::string solution)
{
    int zeros=0;
    for (char& c: solution)
        if (c == '0')
            zeros += 1;
    return zeros;
}

void app()
{
    std::ifstream filu("sudoku/medium.csv");
    //std::ifstream filu("sudoku/sudoku.csv");
    //std::ifstream filu("sudoku/p5.csv");
    std::string line;
    std::getline(filu, line);
    int total=0;
    int classes[3] = {};

    int currtime = cloque();

    while(true)
    {
        std::getline(filu, line);
        if (filu.eof())
            break;

        for(char& c: line)
            if (c == '.')
                c = '0';

        auto sudokudata = Explode(line,',');
        Sudoku sudoku{sudokudata[0], 9};

        std::string solution = sudoku.Solve();

        classes[int(sudoku.smort.GetSolveState())] += 1;
        total += 1;

        //int hm = sudoku.smort.hypotheses_made;
        //cout << hm << " hypotheses made." << endl;

        if (cloque()-currtime >= 1000)
        {
            int fail = classes[0];
            int unknown = classes[1];
            int good = classes[2];

            float goodperc = float(good)/float(total)*100.0f;
            float unknownperc = float(unknown)/float(total)*100.0f;
            float failperc = float(fail)/float(total)*100.0f;

            std::cout << "good " << goodperc << " %, unknown " << unknownperc << " %, fail " << failperc << " %    ";

            std::cout << good << " " << unknown << " " << fail << " " << total << "      \r";
            currtime += 1000;
        }
    }
}

int main()
{
    app();
}
