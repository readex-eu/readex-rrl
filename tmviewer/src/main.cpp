#include <tmm/tuning_model.hpp>

#include <fstream>
#include <iostream>
#include <memory>

void print_help()
{
    std::cout << "tmviewer - print tuning model\n";
    std::cout << "USAGE: tmviewer [OPTIONS] file\n";
    std::cout << "OPTIONS:\n";
    std::cout << "-d\tprint tuning model as dot\n";
    std::cout << "-m\tprint tuning model as matrix\n";
}

class cmdargs final
{
public:
    inline cmdargs() : print_dot(false), print_mat(false)
    {
    }

    bool print_dot;
    bool print_mat;
    std::string file;
};

cmdargs parse_args(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        exit(1);
    }

    cmdargs args;
    for (int n = 1; n < argc - 1; n++)
    {
        std::string arg = argv[n];
        if (arg == "-d")
        {
            args.print_dot = true;
        }
        else if (arg == "-m")
        {
            args.print_mat = true;
        }
        else
        {
            print_help();
            exit(1);
        }
    }
    args.file = argv[argc - 1];

    return args;
}

int main(int argc, char *argv[])
{
    cmdargs args = parse_args(argc, argv);

    rrl::tmm::tuning_model tm;
    if (args.file == "-")
    {
        tm.deserialize(std::cin);
    }
    else
    {
        std::ifstream is(args.file);
        tm.deserialize(is);
    }

    if (args.print_dot)
        std::cout << tm.to_dot();

    if (args.print_mat)
        std::cout << tm.to_matrix();

    return 0;
}
