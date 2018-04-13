#pragma once

#include <fstream>
#include "chip.h"

namespace Utils {

    void random_placement(Chip &chip, std::int64_t num_iter, std::ostream* os = nullptr);
    void simulated_annealing(Chip &chip, std::int64_t num_iter, std::size_t num_swap_per_atom, double hot, double cooling_factor, std::ostream* os = nullptr);
    
    Chip analytical_placement(std::size_t width, std::size_t height, const Netlist &netlist, int num_iter, std::ostream* os = nullptr);

}