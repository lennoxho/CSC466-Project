#include "chip.h"
#include "placement.h"
#include <iostream>

int main() {
    Netlist netlist = Utils::random_netlist(10, 5, 1000, 1000, 3, 3);
    {
        Chip chip{ 2000, 2000, netlist };
        std::cout << chip.get_bbox() << "\n";
        Utils::random_placement(chip, 10000);
        std::cout << chip.get_bbox() << "\n";
    }

    {
        Chip chip{ 2000, 2000, netlist };
        std::cout << chip.get_bbox() << "\n";
        Utils::simulated_annealing(chip, 10, 1, 2, 0.5);
        std::cout << chip.get_bbox() << "\n";
    }

    return 0;
}