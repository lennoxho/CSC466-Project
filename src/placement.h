#pragma once

#include "chip.h"

namespace Utils {

    void random_placement(Chip &chip, std::int64_t num_iter);
    void simulated_annealing(Chip &chip, std::int64_t num_iter, std::size_t num_swap_per_atom, double hot, double cooling_factor);

}