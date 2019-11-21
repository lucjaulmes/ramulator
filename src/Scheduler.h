/***************************** SCHEDULER.H ***********************************
- SAFARI GROUP

This file contains the different scheduling policies and row policies that the 
memory controller can use to schedule requests.

Current Memory Scheduling Policies:

1) FCFS - First Come First Serve
        This scheduling policy schedules memory requests chronologically

2) FRFCFS - Frist Ready First Come First Serve
        This scheduling policy first checks if a request is READY(meets all 
        timing parameters), if yes then it is prioritized. If multiple requests
        are ready, they they are scheduled chronologically. Otherwise, it 
        behaves the same way as FCFS. 

3) FRFCFS_Cap - First Ready First Come First Serve Cap
       This scheduling policy behaves the same way as FRFCS, except that it has
       a cap on the number of hits you can get in a certain row. The CAP VALUE
       can be altered by changing the number for the "cap" variable in 
       line number 76. 

4) FRFCFS_PriorHit - First Ready First Come First Serve Prioritize Hits
       This scheduling policy behaves the same way as FRFCFS, except that it
       prioritizes row hits more than readiness. 

You can select which scheduler you want to use by changing the value of 
"type" variable on line number 74.

                _______________________________________

Current Row Policies:

1) Closed   - Precharges a row as soon as there are no pending references to 
              the active row.
2) ClosedAP - Closed Auto Precharge
3) Opened   - Precharges a row only if there are pending references to 
              other rows.
4) Timeout  - Precharges a row after X time if there are no pending references.
              'X' time can be changed by changing the variable timeout 
              on line number 221

*****************************************************************************/

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "DRAM.h"
#include "Request.h"
#include "Config.h"
#include "Controller.h"
#include <vector>
#include <map>
#include <string>
#include <list>
#include <functional>
#include <algorithm>
#include <cassert>


namespace ramulator
{

template <typename T>
class Controller;

template <typename T>
class Scheduler
{
public:
    Controller<T>* ctrl;

    enum class Type {
        FCFS, FRFCFS, FRFCFS_Cap, FRFCFS_PriorHit, MAX
    } type = Type::FRFCFS_Cap;

    long cap = 16; //Change this line to change cap

    Scheduler(Controller<T>* ctrl, std::string policy) : ctrl(ctrl)
    {
        std::transform(policy.begin(), policy.end(), policy.begin(), [](unsigned char c){ return std::tolower(c); });
        if (policy == "fcfs") {
            type = Type::FCFS;
        } else if (policy == "frfcfs") {
            type = Type::FRFCFS;
        } else if (policy == "frfcfs_cap") {
            type = Type::FRFCFS_Cap;
        } else if (policy == "frfcfs_timeout") {
            type = Type::FRFCFS_PriorHit;
        } else if (policy != Config::missing) {
            std::cerr << "Warning: unrecognized scheduler type \"" << policy << "\"\n";
        }
    }

    std::list<Request>::iterator get_head(std::list<Request>& q)
    {
        // TODO make the decision at compile time
        if (type != Type::FRFCFS_PriorHit) {
            //If queue is empty, return end of queue
            if (!q.size())
                return q.end();

            //Else return based on the policy
            auto head = q.begin();
            for (auto itr = next(q.begin(), 1); itr != q.end(); itr++)
                head = compare[int(type)](head, itr);

            return head;
        } 
        else { //Code to get around edge cases for FRFCFS_PriorHit
            
       //If queue is empty, return end of queue
            if (!q.size())
                return q.end();

       //Else return based on FRFCFS_PriorHit Scheduling Policy
            auto head = q.begin();
            for (auto itr = next(q.begin(), 1); itr != q.end(); itr++) {
                head = compare[int(Type::FRFCFS_PriorHit)](head, itr);
            }

            if (this->ctrl->is_ready(head) && this->ctrl->is_row_hit(head)) {
                return head;
            }

            // prepare a list of hit request
            std::vector<std::vector<int>> hit_reqs;
            for (auto itr = q.begin() ; itr != q.end() ; ++itr) {
                if (this->ctrl->is_row_hit(itr)) {
                    auto begin = itr->addr_vec.begin();
                    // TODO Here it assumes all DRAM standards use PRE to close a row
                    // It's better to make it more general.
                    auto end = begin + int(ctrl->channel->spec->scope[int(T::Command::PRE)]) + 1;
                    std::vector<int> rowgroup(begin, end); // bank or subarray
                    hit_reqs.push_back(rowgroup);
                }
            }
            // if we can't find proper request, we need to return q.end(),
            // so that no command will be scheduled
            head = q.end();
            for (auto itr = q.begin(); itr != q.end(); itr++) {
                bool violate_hit = false;
                if ((!this->ctrl->is_row_hit(itr)) && this->ctrl->is_row_open(itr)) {
                    // so the next instruction to be scheduled is PRE, might violate hit
                    auto begin = itr->addr_vec.begin();
                    // TODO Here it assumes all DRAM standards use PRE to close a row
                    // It's better to make it more general.
                    auto end = begin + int(ctrl->channel->spec->scope[int(T::Command::PRE)]) + 1;
                    std::vector<int> rowgroup(begin, end); // bank or subarray
                    for (const auto& hit_req_rowgroup : hit_reqs) {
                        if (rowgroup == hit_req_rowgroup) {
                            violate_hit = true;
                            break;
                        }
                    }
                }
                if (violate_hit) {
                    continue;
                }
                // If it comes here, that means it won't violate any hit request
                if (head == q.end()) {
                    head = itr;
                } else {
                    head = compare[int(Type::FRFCFS)](head, itr);
                }
            }

            return head;
        }
    }

//Compare functions for each memory schedulers
private:
    typedef std::list<Request>::iterator ReqIter;
    std::function<ReqIter(ReqIter, ReqIter)> compare[int(Type::MAX)] = {
        // FCFS
        [this] (ReqIter req1, ReqIter req2) {
            if (req1->arrive <= req2->arrive) return req1;
            return req2;},

        // FRFCFS
        [this] (ReqIter req1, ReqIter req2) {
            bool ready1 = this->ctrl->is_ready(req1);
            bool ready2 = this->ctrl->is_ready(req2);

            if (ready1 ^ ready2) {
                if (ready1) return req1;
                return req2;
            }

            if (req1->arrive <= req2->arrive) return req1;
            return req2;},

        // FRFCFS_CAP
        [this] (ReqIter req1, ReqIter req2) {
            bool ready1 = this->ctrl->is_ready(req1);
            bool ready2 = this->ctrl->is_ready(req2);

            ready1 = ready1 && (this->ctrl->rowtable->get_hits(req1->addr_vec) <= this->cap);
            ready2 = ready2 && (this->ctrl->rowtable->get_hits(req2->addr_vec) <= this->cap);

            if (ready1 ^ ready2) {
                if (ready1) return req1;
                return req2;
            }

            if (req1->arrive <= req2->arrive) return req1;
            return req2;},
        // FRFCFS_PriorHit
        [this] (ReqIter req1, ReqIter req2) {
            bool ready1 = this->ctrl->is_ready(req1) && this->ctrl->is_row_hit(req1);
            bool ready2 = this->ctrl->is_ready(req2) && this->ctrl->is_row_hit(req2);

            if (ready1 ^ ready2) {
                if (ready1) return req1;
                return req2;
            }

            if (req1->arrive <= req2->arrive) return req1;
            return req2;}
    };
};


// Row Precharge Policy
template <typename T>
class RowPolicy
{
public:
    Controller<T>* ctrl;

    enum class Type {
        Closed, ClosedAP, Opened, Timeout, MAX
    } type = Type::Opened;

    int timeout = 50;

    RowPolicy(Controller<T>* ctrl, std::string policy) : ctrl(ctrl)
    {
        std::transform(policy.begin(), policy.end(), policy.begin(), [](unsigned char c){ return std::tolower(c); });
        if (policy == "closed") {
            type = Type::Closed;
        } else if (policy == "closedap") {
            type = Type::ClosedAP;
        } else if (policy == "opened") {
            type = Type::Opened;
        } else if (policy == "timeout") {
            type = Type::Timeout;
        } else if (policy != Config::missing) {
            std::cerr << "Warning: unrecognized row policy \"" << policy << "\"\n";
        }
    }

    std::vector<int> get_victim(typename T::Command cmd)
    {
        return policy[int(type)](cmd);
    }

private:
    std::function<std::vector<int>(typename T::Command)> policy[int(Type::MAX)] = {
        // Closed
        [this] (typename T::Command cmd) -> std::vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return std::vector<int>();},

        // ClosedAP
        [this] (typename T::Command cmd) -> std::vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return std::vector<int>();},

        // Opened
        [this] (typename T::Command cmd) {
            return std::vector<int>();},

        // Timeout
        [this] (typename T::Command cmd) -> std::vector<int> {
            for (auto& kv : this->ctrl->rowtable->table) {
                auto& entry = kv.second;
                if (this->ctrl->clk - entry.timestamp < timeout)
                    continue;
                if (!this->ctrl->is_ready(cmd, kv.first))
                    continue;
                return kv.first;
            }
            return std::vector<int>();}
    };

};


template <typename T>
class RowTable
{
public:
    Controller<T>* ctrl;

    struct Entry {
        int row;
        int hits;
        long timestamp;
    };

    std::map<std::vector<int>, Entry> table;

    RowTable(Controller<T>* ctrl) : ctrl(ctrl) {}

    void update(typename T::Command cmd, const std::vector<int>& addr_vec, long clk)
    {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);
        std::vector<int> rowgroup(begin, end); // bank or subarray
        int row = *end;

        T* spec = ctrl->channel->spec;

        if (spec->is_opening(cmd))
            table.insert({rowgroup, {row, 0, clk}});

        if (spec->is_accessing(cmd)) {
            // we are accessing a row -- update its entry
            auto match = table.find(rowgroup);
            assert(match != table.end());
            assert(match->second.row == row);
            match->second.hits++;
            match->second.timestamp = clk;
        } /* accessing */

        if (spec->is_closing(cmd)) {
          // we are closing one or more rows -- remove their entries
          int n_rm = 0;
          int scope;
          if (spec->is_accessing(cmd))
            scope = int(T::Level::Row) - 1; //special condition for RDA and WRA
          else
            scope = int(spec->scope[int(cmd)]);

          for (auto it = table.begin(); it != table.end();) {
            if (equal(begin, begin + scope + 1, it->first.begin())) {
              n_rm++;
              it = table.erase(it);
            }
            else
              it++;
          }

          assert(n_rm > 0);
        } /* closing */
    }

    int get_hits(const std::vector<int>& addr_vec, const bool to_opened_row = false)
    {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);

        std::vector<int> rowgroup(begin, end);
        int row = *end;

        auto itr = table.find(rowgroup);
        if (itr == table.end())
            return 0;

        if(!to_opened_row && (itr->second.row != row))
            return 0;

        return itr->second.hits;
    }

    int get_open_row(const std::vector<int>& addr_vec) {
        auto begin = addr_vec.begin();
        auto end = begin + int(T::Level::Row);

        std::vector<int> rowgroup(begin, end);

        auto itr = table.find(rowgroup);
        if(itr == table.end())
            return -1;

        return itr->second.row;
    }
};

} /*namespace ramulator*/

#endif /*__SCHEDULER_H*/
