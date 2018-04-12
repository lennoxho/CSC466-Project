#include "chip.h"
#include "placement.h"
#include <iostream>

int main() {
    Netlist netlist = Utils::random_netlist(10, 5, 1000, 1000, 3, 3);
    Utils::dump_netlist(netlist, "netlist.out");

    {
        std::ofstream hfile{ "random.out", std::ios::out | std::ios::binary };
        RUNTIME_ASSERT(hfile);
    
        Chip chip{ 45, 45, netlist };
        std::cout << chip.get_bbox() << "\n";
        Utils::random_placement(chip, 40000, &hfile);
        std::cout << chip.get_bbox() << "\n";
    
        RUNTIME_ASSERT(hfile);
    }

    {
        std::ofstream hfile{ "sim.out", std::ios::out | std::ios::binary };
        RUNTIME_ASSERT(hfile);

        Chip chip{ 45, 45, netlist };
        Utils::simulated_annealing(chip, 10, 2, 0.5, 0.5, &hfile);
        std::cout << chip.get_bbox() << "\n";

        RUNTIME_ASSERT(hfile);
    }

    return 0;
}