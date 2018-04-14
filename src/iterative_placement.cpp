#include <random>
#include "placement.h"

namespace Utils {

    void dump_chip(const Chip &chip, std::ostream &os) {
        for (const auto &entry : chip.coords()) {
            os << "(" << entry.x << "," << entry.y << ")\n";
        }
    }

    void random_placement(Chip &chip, std::int64_t num_iter, metric_consumer* met) {
        std::mt19937 eng;
        std::bernoulli_distribution type_dist;

        std::uniform_int_distribution<std::size_t> chip_dist{ 0, chip.get_width()*chip.get_height()/2 - 1 };
        std::uniform_int_distribution<std::size_t> lut_dist{ 0, chip.get_netlist().num_luts() - 1 };
        std::uniform_int_distribution<std::size_t> ff_dist{ 0, chip.get_netlist().num_ffs() - 1 };

        if (met != nullptr) {
            met->snapshot() << "ss " << 0 << " (" << chip.get_width() << "," << chip.get_height() << "):\n";
            dump_chip(chip, met->snapshot());
        }

        for (std::int64_t i = 0; i < num_iter; ++i) {
            std::int64_t prev_bbox = chip.get_bbox();
            if (met != nullptr) {
                met->iter() << prev_bbox << "\n";
            }

            const Atom &atom_to_swap = type_dist(eng) ? get<Netlist::LUT>(chip.get_netlist(), lut_dist(eng)) :
                                                        get<Netlist::FF>(chip.get_netlist(), ff_dist(eng));
            std::size_t new_idx = chip_dist(eng);
            std::size_t prev_idx = chip.swap(atom_to_swap, new_idx);

            if (chip.get_bbox() > prev_bbox) {
                chip.swap(atom_to_swap, prev_idx);
            }
        }

        if (met != nullptr) {
            met->snapshot() << "ss " << 0 << " (" << chip.get_width() << "," << chip.get_height() << "):\n";
            dump_chip(chip, met->snapshot());
        }
    }

    void simulated_annealing(Chip &chip, std::int64_t num_iter, std::size_t num_swap_per_temperature, double hot, double cooling_factor, metric_consumer* met) {
        std::mt19937 eng;
        std::bernoulli_distribution type_dist;

        std::uniform_int_distribution<std::size_t> chip_dist{ 0, chip.get_width()*chip.get_height() / 2 - 1 };
        std::uniform_int_distribution<std::size_t> lut_dist{ 0, chip.get_netlist().num_luts() - 1 };
        std::uniform_int_distribution<std::size_t> ff_dist{ 0, chip.get_netlist().num_ffs() - 1 };
        std::uniform_real_distribution<double> unif{ 0.0, 1.0 };

        if (met != nullptr) {
            met->snapshot() << "ss " << 0 << " (" << chip.get_width() << "," << chip.get_height() << "):\n";
            dump_chip(chip, met->snapshot());
        }

        double temperature = hot;
        for (std::int64_t i = 0; i < num_iter; ++i) {
            for (std::size_t j = 0; j < num_swap_per_temperature; ++j) {
                std::int64_t prev_bbox = chip.get_bbox();
                if (met != nullptr) {
                    met->iter() << prev_bbox << "\n";
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

        if (met != nullptr) {
            met->snapshot() << "ss " << 0 << " (" << chip.get_width() << "," << chip.get_height() << "):\n";
            dump_chip(chip, met->snapshot());
        }
    }

}