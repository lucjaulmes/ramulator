#ifndef __GEM5_WRAPPER_H
#define __GEM5_WRAPPER_H

#include <string>
#include <map>
#include <functional>

#include "Config.h"

using namespace std;

namespace ramulator
{

class Request;
class MemoryBase;

class Gem5Wrapper 
{
private:
    MemoryBase *mem;
public:
    double tCK;
    Gem5Wrapper(const Config& configs, int cacheline);
    ~Gem5Wrapper();
    void tick();
    bool send(Request req);
    void finish(void);
};

extern std::map<std::string, std::function<MemoryBase *(const Config &, int)> > name_to_func;

} /*namespace ramulator*/

#endif /*__GEM5_WRAPPER_H*/
