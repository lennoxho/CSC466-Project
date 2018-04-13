#include "chip.h"
#include "placement.h"
#include <iostream>

int main() {
    Netlist netlist = Utils::random_netlist(10, 5, 1000, 1000, 3, 3);
    Utils::dump_netlist(netlist, "netlist.out");

    const std::size_t width = 100;
    const std::size_t height = 100;

    {
        Utils::metric_consumer met{ "random_iter.out", "random_ss.out" };
    
        Chip chip{ width, height, netlist };
        std::cout << chip.get_bbox() << "\n";
        Utils::random_placement(chip, 40000, &met);
        std::cout << chip.get_bbox() << "\n";
    
        RUNTIME_ASSERT(met);
    }

    {
        Utils::metric_consumer met{ "sim_iter.out", "sim_ss.out" };

        Chip chip{ width, height, netlist };
        Utils::simulated_annealing(chip, 10, 2, 0.5, 0.5, &met);
        std::cout << chip.get_bbox() << "\n";

        RUNTIME_ASSERT(met);
    }

    {
        Utils::metric_consumer met{ "qp_iter.out", "qp_ss.out" };

        Plan plan{ Utils::quadratic_placement(width, height, netlist, 2, &met) };

        Chip chip_rand{ plan };
        Utils::random_placement(chip_rand, 40000);
        std::cout << chip_rand.get_bbox() << "\n";

        Chip chip_sim{ plan };
        Utils::simulated_annealing(chip_sim, 10, 2, 0.5, 0.5, &met);
        std::cout << chip_sim.get_bbox() << "\n";

        RUNTIME_ASSERT(met);
    }

    return 0;
}