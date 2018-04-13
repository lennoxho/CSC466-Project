#include <boost/range/combine.hpp>
#include <unordered_set>
#include "plan.h"
#include "placement.h"

namespace Utils {

    Chip analytical_placement(std::size_t width, std::size_t height, const Netlist &netlist, int num_iter, std::ostream* os) {
        Plan plan{ width, height, netlist };
        
        bool split_horizontally = true;
        for (int i = 0; i < num_iter; ++i) {
            if (i > 0) {
                plan.recursive_partition(split_horizontally);
                split_horizontally = !split_horizontally;
            }
            
            for (const auto &entry : boost::combine(plan.partitions(), plan.bounds())) {
                const Plan::Partition &partition = boost::get<0>(entry);
                const Plan::plan_region &region = boost::get<1>(entry);
                
                std::unordered_set<const Atom*> atoms_in_partition{ partition.begin(), partition.end() };
                std::vector<Plan::coord> solution(partition.size());
                
                // TODO
            }
        }
        
        return Chip{ plan };
    }

}