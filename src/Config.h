#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

namespace ramulator
{

class Config {

private:
    map<string, string> options;
    int channels;
    int ranks;
    int subarrays;
    int cpu_tick;
    int mem_tick;
    int core_num = 0;
    long expected_limit_insts = 0;
    long warmup_insts = 0;

public:
    static const string missing;

    Config() {}
    Config(const string& fname);
    void parse(const string& fname);
    void add(const string& name, const string& value);

    const string& operator [](const string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second;
        } else {
            return missing;
        }
    }

    bool contains(const string& name) const {
        return options.find(name) != options.end();
    }

    void set_core_num(int _core_num) {core_num = _core_num;}

    int get_channels() const {return channels;}
    int get_subarrays() const {return subarrays;}
    int get_ranks() const {return ranks;}
    int get_cpu_tick() const {return cpu_tick;}
    int get_mem_tick() const {return mem_tick;}
    int get_core_num() const {return core_num;}
    long get_expected_limit_insts() const {return expected_limit_insts;}
    long get_warmup_insts() const {return warmup_insts;}

    bool has_l3_cache() const {
        const string& cache_option = (*this)["cache"];
        return (cache_option == "all") || (cache_option == "L3");
    }

    bool has_core_caches() const {
        const string& cache_option = (*this)["cache"];
        return (cache_option == "all" || cache_option == "L1L2");
    }

    bool is_early_exit() const {
        // the default value is true
        const string& exit_option = (*this)["early_exit"];
        return exit_option == "off";
    }

    bool calc_weighted_speedup() const {
        return (expected_limit_insts != 0);
    }

    bool record_cmd_trace() const {
        // the default value is false
        const string& record_option = (*this)["record_cmd_trace"];
        return record_option == "on";
    }

    bool print_cmd_trace() const {
        // the default value is false
        const string& print_option = (*this)["print_cmd_trace"];
        return print_option == "on";
    }
};


} /* namespace ramulator */

#endif /* _CONFIG_H */

