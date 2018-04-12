#include <random>
#include "placement.h"

namespace Utils {

    void random_placement(Chip &chip, std::int64_t num_iter, std::ostream* os) {
        std::mt19937 eng;
        std::bernoulli_distribution type_dist;

        std::uniform_int_distribution<std::size_t> chip_dist{ 0, chip.get_width()*chip.get_height()/2 - 1 };
        std::uniform_int_distribution<std::size_t> lut_dist{ 0, chip.get_netlist().num_luts() - 1 };
        std::uniform_int_distribution<std::size_t> ff_dist{ 0, chip.get_netlist().num_ffs() - 1 };

        for (std::int64_t i = 0; i < num_iter; ++i) {
            std::int64_t prev_bbox = chip.get_bbox();
            if (os != nullptr) {
                *os << prev_bbox << "\n";
            }

            const Atom &atom_to_swap = type_dist(eng) ? get<Netlist::LUT>(chip.get_netlist(), lut_dist(eng)) :
                                                        get<Netlist::FF>(chip.get_netlist(), ff_dist(eng));
            std::size_t new_idx = chip_dist(eng);
            std::size_t prev_idx = chip.swap(atom_to_swap, new_idx);

            if (chip.get_bbox() > prev_bbox) {
                chip.swap(atom_to_swap, prev_idx);
            }
        }
    }

    void simulated_annealing(Chip &chip, std::int64_t num_iter, std::size_t num_swap_per_atom, double hot, double cooling_factor, std::ostream* os) {
        std::mt19937 eng;
        std::bernoulli_distribution type_dist;

        std::uniform_int_distribution<std::size_t> chip_dist{ 0, chip.get_width()*chip.get_height() / 2 - 1 };
        std::uniform_int_distribution<std::size_t> lut_dist{ 0, chip.get_netlist().num_luts() - 1 };
        std::uniform_int_distribution<std::size_t> ff_dist{ 0, chip.get_netlist().num_ffs() - 1 };
        std::uniform_real_distribution<double> unif{ 0.0, 1.0 };
        std::size_t num_atoms = chip.get_netlist().num_luts() + chip.get_netlist().num_ffs();

        double temperature = hot;
        for (std::int64_t i = 0; i < num_iter; ++i) {
            for (std::size_t j = 0; j < num_swap_per_atom * num_atoms; ++j) {
                std::int64_t prev_bbox = chip.get_bbox();
                if (os != nullptr) {
                    *os << prev_bbox << "\n";
                }

                const Atom &atom_to_swap = type_dist(eng) ? get<Netlist::LUT>(chip.get_netlist(), lut_dist(eng)) :
                    get<Netlist::FF>(chip.get_netlist(), ff_dist(eng));
                std::size_t new_idx = chip_dist(eng);
                std::size_t prev_idx = chip.swap(atom_to_swap, new_idx);

                if (chip.get_bbox() > prev_bbox &&
                    unif(eng) >= std::exp(static_cast<double>(prev_bbox - chip.get_bbox()) / temperature))
                {
                    chip.swap(atom_to_swap, prev_idx);
                }
            }

            temperature *= cooling_factor;
        }
    }

}