#include "Global.h"
#include "Util.h"
#include "Smort.h"

struct Sudoku
{
    int size;
    std::vector<int> clues;
    Smort smort;

    void MakeConstraints()
    {
        smort.n_classes = size;
        //constraint #0 is the world
        smort.constraints.push_back(Constraint{Constraint::TYPE::WORLD, size*size, size});
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
                    area.constraint_id = smort.constraints.size();

                    for(int dy=starty; dy<starty+Y; ++dy)
                    {
                        for(int dx=startx; dx<startx+X; ++dx)
                        {
                            world.cell_ids.push_back(dy*size+dx);
                            area.cell_ids.push_back(area.cell_ids.size());
                        }
                    }
                    Constraint c{Constraint::TYPE::ALL_DIFFERENT, size, size};
                    smort.constraints.push_back(c);
                    smort.relations.push_back(Relation{std::make_pair(world, area)});
                }
            }
        }
    }

    std::string Solve()
    {
        //fill in the values
        smort_assert(clues.size() == size*size);
        for(int i=0; i<size*size; ++i)
        {
            if (clues[i] == -1)
                continue;

            int y = i/size;
            int x = i%size;

            int clue = clues[i];
            //constraint #0 is the world
            auto view = smort.constraints[0].View(MakeVector(size_t(y*size+x)))[0];
            Ops::SetOneTrue(view, clue, size);
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
                    for(int k=0; k<size; ++k)
                    {
                        int n = smort.constraints[0].data[i*81+j*9+k];
                        if (n)
                            sum+=1, last=k;
                    }
                    if (sum==1)
                        solution[i*9+j] = '1'+last;
                }
            }
        }
        return solution;
    }

    Sudoku(const std::string& str, int size_):size(size_)
    {
        smort_assert(size*size == str.length());
        MakeConstraints();
        for(const char& c: str)
        {
            if (c == '0')
                clues.push_back(-1);
            else
                clues.push_back(c-'1');
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
    //std::ifstream filu("sudoku/medium.csv");
    std::ifstream filu("sudoku/sudoku.csv");
    //std::ifstream filu("sudoku/p5.csv");
    std::string line;
    std::getline(filu, line);
    int total=0, good=0;

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
        //if (solution == sudokudata[1])
        if (numzeros(solution) == 0)
        {
            good += 1;
        }
        total += 1;

        if (cloque()-currtime >= 1000)
        {
            int bad = total-good;

            float goodperc = float(good)/float(total)*100.0f;
            float badperc = float(bad)/float(total)*100.0f;

            std::cout << "good " << goodperc << " %, bad " << badperc << " %,      " << good << " " << bad << " " << total << "      \r";
            currtime += 1000;
        }
    }
}

int main()
{
    app();
}
