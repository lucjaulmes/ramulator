#include "Config.h"

using namespace std;
using namespace ramulator;

const string ramulator::Config::missing = "";

Config::Config(const string &fname)
{
    parse(fname);
}

void Config::add(const string &name, const string &value)
{
    if (contains(name))
    {
        std::cerr << "ramulator::Config::add options[" << name << "] already set.\n";
        return;
    }

    options.insert(make_pair(name, value));

    if (name == "channels")
        channels = stoi(value);
    else if (name == "ranks")
        ranks = stoi(value);
    else if (name == "subarrays")
        subarrays = stoi(value);
    else if (name == "cpu_tick")
        cpu_tick = stoi(value);
    else if (name == "mem_tick")
        mem_tick = stoi(value);
    else if (name == "expected_limit_insts")
        expected_limit_insts = stol(value);
    else if (name == "warmup_insts")
        warmup_insts = stol(value);
}

void Config::parse(const string &fname)
{
    ifstream file(fname);
    if (!file.good()) {
        std::cerr << "Bad config file \"" << fname << "\"\n";
        std::exit(1);
    }

    string line;
    while (getline(file, line))
    {
        char delim[] = " \t=";
        vector<string> tokens;

        for (size_t start = line.find_first_not_of(delim), end; start != string::npos;
                    start = line.find_first_not_of(delim, end))
        {
            end = line.find_first_of(delim, start);
            tokens.push_back(line.substr(start, end));
        }

        // empty or comment line
        if (!tokens.size() || tokens[0][0] == '#')
            continue;

        // unexpected parameter line
        else if (tokens.size() != 2) {
            std::cerr << "Only allow two tokens in one line: " << line << '\n';
            std::exit(2);
        }

        // correct parameter line
        else
            add(tokens[0], tokens[1]);
    }
    file.close();
}


