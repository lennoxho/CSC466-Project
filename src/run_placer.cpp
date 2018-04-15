#include "chip.h"
#include "placement.h"
#include <iostream>

void run_demos() {
    constexpr std::size_t num_atoms = 1000;
    constexpr std::size_t num_iterations = 100'000;

    Netlist netlist = Utils::random_netlist(10, 5, num_atoms, num_atoms, 3, 3, 1);
    Utils::dump_netlist(netlist, "10_5_1000_1000_3_3_1_netlist.out");

    Chip chip{ 100, 100, netlist };

    {
        Utils::metric_consumer met{ "rand_iter.out", "rand_ss.out" };
        Chip chip_clone{ chip.clone() };
        Utils::random_placement(chip_clone, num_iterations, &met);
        RUNTIME_ASSERT(met);
    }

    {
        Utils::metric_consumer met{ "sim_iter.out", "sim_ss.out" };
        Chip chip_clone{ chip.clone() };
        Utils::simulated_annealing(chip_clone, 5, num_iterations / 5, 0.5, 0.5, &met);
        RUNTIME_ASSERT(met);
    }

    {
        Utils::metric_consumer met{ "qp_adaptive_iter.out", "qp_adaptive_ss.out" };

        Plan plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 4,
            Plan::partitioning_method::adaptive, 1, &met) };
        Chip qp_chip{ plan };

        Chip qp_rand_chip{ qp_chip.clone() };
        Utils::random_placement(qp_rand_chip, num_iterations, &met);

        Utils::simulated_annealing(qp_chip, 5, num_iterations / 5, 0.5, 0.5, &met);
    }

    {
        Utils::metric_consumer met{ "qp_bisection_iter.out", "qp_bisection_ss.out" };

        Plan plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 4,
            Plan::partitioning_method::bisection, 1, &met) };
        Chip qp_chip{ plan };

        Chip qp_rand_chip{ qp_chip.clone() };
        Utils::random_placement(qp_rand_chip, num_iterations, &met);

        Utils::simulated_annealing(qp_chip, 5, num_iterations / 5, 0.5, 0.5, &met);
    }

    Netlist netlist_3_phases = Utils::random_netlist(10, 5, num_atoms, num_atoms, 3, 3, 3);
    Utils::dump_netlist(netlist_3_phases, "10_5_1000_1000_3_3_3_netlist.out");

    {
        Utils::metric_consumer met{ "qp_adaptive_3_phases_iter.out", "qp_adaptive_3_phases_ss.out" };

        Plan plan{ Utils::quadratic_placement(100, 100, netlist_3_phases, 4,
            Plan::partitioning_method::adaptive, 3, &met) };
    }

    {
        Utils::metric_consumer met{ "qp_bisection_3_phases_iter.out", "qp_bisection_3_phases_ss.out" };

        Plan plan{ Utils::quadratic_placement(100, 100, netlist_3_phases, 4,
            Plan::partitioning_method::bisection, 3, &met) };
    }
}

void run_num_iterations_experiments(const Chip &chip) {
    constexpr std::size_t iter_begin = 100;
    constexpr std::size_t iter_end = 100'000;
    constexpr std::size_t iter_factor = 10;

    std::cout << chip.get_netlist() << " Netlist, ("
        << chip.get_width() << ", " << chip.get_height() << ") Chip.\n";

    for (std::size_t i = iter_begin; i <= iter_end; i *= iter_factor) {
        std::cout << "Random placement with " << i << " iterations. BBOX = ";

        Chip expr_chip{ chip.clone() };
        Utils::random_placement(expr_chip, i, nullptr);
        std::cout << expr_chip.get_bbox() << "\n";
    }

    for (std::size_t i = iter_begin; i <= iter_end; i *= iter_factor) {
        std::cout << "Simulated annealing with " << i << " iterations. BBOX = ";

        Chip expr_chip{ chip.clone() };
        Utils::simulated_annealing(expr_chip, 5, i / 5, 0.5, 0.5, nullptr);
        std::cout << expr_chip.get_bbox() << "\n";
    }
}

void run_num_recursions_experiments(const Chip &chip, std::size_t num_phases) {
    constexpr std::size_t iter_begin = 1;
    constexpr std::size_t iter_end = 4;

    std::cout << chip.get_netlist() << " Netlist, ("
        << chip.get_width() << ", " << chip.get_height() << ") Chip.\n";

    for (std::size_t i = iter_begin; i <= iter_end; ++i) {
        std::cout << "Quadratic placement + adaptive partitioning with " << i << " recursions. BBOX = ";

        Plan plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), i,
            Plan::partitioning_method::adaptive, num_phases, nullptr) };
        Chip expr_chip{ plan };
        std::cout << expr_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations random placement with " << i << " recursions. BBOX = ";
        Chip rand_chip{ expr_chip.clone() };
        Utils::random_placement(rand_chip, 10'000, nullptr);
        std::cout << rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations simulated annealing with " << i << " recursions. BBOX = ";
        Utils::simulated_annealing(expr_chip, 5, 2000, 0.5, 0.5, nullptr);
        std::cout << expr_chip.get_bbox() << "\n";
    }

    for (std::size_t i = iter_begin; i <= iter_end; ++i) {
        std::cout << "Quadratic placement + bisection partitioning with " << i << " recursions. BBOX = ";

        Plan plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), i,
            Plan::partitioning_method::bisection, num_phases, nullptr) };
        Chip expr_chip{ plan };
        std::cout << expr_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 100000 iterations random placement with " << i << " recursions. BBOX = ";
        Chip rand_chip{ expr_chip.clone() };
        Utils::random_placement(rand_chip, 10'000, nullptr);
        std::cout << rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 100000 iterations simulated annealing with " << i << " recursions. BBOX = ";
        Utils::simulated_annealing(expr_chip, 5, 2000, 0.5, 0.5, nullptr);
        std::cout << expr_chip.get_bbox() << "\n";
    }
}

void run_num_phases_experiments() {
    constexpr std::size_t iter_begin = 1;
    constexpr std::size_t iter_end = 10;
    constexpr std::size_t num_iterations = 10'000;

    for (std::size_t i = iter_begin; i <= iter_end; ++i) {
        Netlist netlist = Utils::random_netlist(10, 5, 1000, 1000, 3, 3, i);
        Chip chip{ 100, 100, netlist };

        std::cout << "Random placement with " << std::to_string(i) << " phases. BBOX = ";
        Chip rand_chip{ chip.clone() };
        Utils::random_placement(rand_chip, num_iterations, nullptr);
        std::cout << rand_chip.get_bbox() << "\n";

        std::cout << "Simulated annealing with " << std::to_string(i) << " phases. BBOX = ";
        Chip sim_chip{ chip.clone() };
        Utils::simulated_annealing(sim_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << sim_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning with " << std::to_string(i) << " phases. BBOX = ";
        Plan adaptive_plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 2,
            Plan::partitioning_method::adaptive, i, nullptr) };
        Chip adaptive_chip{ adaptive_plan };
        std::cout << adaptive_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations random placement with " << std::to_string(i) << " phases. BBOX = ";
        Chip adaptive_rand_chip{ adaptive_chip.clone() };
        Utils::random_placement(adaptive_rand_chip, num_iterations, nullptr);
        std::cout << adaptive_rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations simulated annealing with " << std::to_string(i) << " phases. BBOX = ";
        Utils::simulated_annealing(adaptive_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << adaptive_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning with " << std::to_string(i) << " phases. BBOX = ";
        Plan bisection_plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 2,
            Plan::partitioning_method::bisection, i, nullptr) };
        Chip bisection_chip{ bisection_plan };
        std::cout << bisection_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 10000 iterations random placement with " << std::to_string(i) << " phases. BBOX = ";
        Chip bisection_rand_chip{ bisection_chip.clone() };
        Utils::random_placement(bisection_rand_chip, num_iterations, nullptr);
        std::cout << bisection_rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 10000 iterations simulated annealing with " << std::to_string(i) << " phases. BBOX = ";
        Utils::simulated_annealing(bisection_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << bisection_chip.get_bbox() << "\n";
    }
}

void run_num_atoms_experiments() {
    constexpr std::size_t iter_begin = 100;
    constexpr std::size_t iter_end = 1'000;
    constexpr std::size_t iter_factor = 10;
    constexpr std::size_t num_iterations = 10'000;
    constexpr std::size_t num_phases = 3;

    for (std::size_t i = iter_begin; i <= iter_end; i*=iter_factor) {
        Netlist netlist = Utils::random_netlist(10, 5, i, i, 3, 3, num_phases);
        Chip chip{ 150, 150, netlist };

        std::cout << "Random placement with " << netlist << " netlist. BBOX = ";
        Chip rand_chip{ chip.clone() };
        Utils::random_placement(rand_chip, num_iterations, nullptr);
        std::cout << rand_chip.get_bbox() << "\n";

        std::cout << "Simulated annealing with " << netlist << " netlist. BBOX = ";
        Chip sim_chip{ chip.clone() };
        Utils::simulated_annealing(sim_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << sim_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning with " << netlist << " netlist. BBOX = ";
        Plan adaptive_plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 2,
            Plan::partitioning_method::adaptive, num_phases, nullptr) };
        Chip adaptive_chip{ adaptive_plan };
        std::cout << adaptive_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations random placement with " << netlist << " netlist. BBOX = ";
        Chip adaptive_rand_chip{ adaptive_chip.clone() };
        Utils::random_placement(adaptive_rand_chip, num_iterations, nullptr);
        std::cout << adaptive_rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + adaptive partitioning + 10000 iterations simulated annealing with " << netlist << " netlist. BBOX = ";
        Utils::simulated_annealing(adaptive_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << adaptive_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning with " << netlist << " netlist. BBOX = ";
        Plan bisection_plan{ Utils::quadratic_placement(chip.get_width(), chip.get_height(), chip.get_netlist(), 2,
            Plan::partitioning_method::bisection, num_phases, nullptr) };
        Chip bisection_chip{ bisection_plan };
        std::cout << bisection_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 10000 iterations random placement with " << netlist << " netlist. BBOX = ";
        Chip bisection_rand_chip{ bisection_chip.clone() };
        Utils::random_placement(bisection_rand_chip, num_iterations, nullptr);
        std::cout << bisection_rand_chip.get_bbox() << "\n";

        std::cout << "Quadratic placement + bisection partitioning + 10000 iterations simulated annealing with " << netlist << " netlist. BBOX = ";
        Utils::simulated_annealing(bisection_chip, 5, num_iterations / 5, 0.5, 0.5, nullptr);
        std::cout << bisection_chip.get_bbox() << "\n";
    }
}

int main() {
    std::cout << "Running demo...\n";
    run_demos();
    std::cout << "\n";

    {
        const std::size_t num_phases = 3;
        Netlist netlist = Utils::random_netlist(10, 5, 1000, 1000, 3, 3, num_phases);

        Chip chip{ 100, 100, netlist };

        std::cout << "Performing number of iterations experiment on iterative placement methods:\n"
            << "----------------------------------------------------------------------------\n";
        run_num_iterations_experiments(chip);
        std::cout << "\n";

        std::cout << "Performing number of recursions experiment on quadratic placement method:\n"
            << "----------------------------------------------------------------------------\n";
        run_num_recursions_experiments(chip, num_phases);
        std::cout << "\n";
    }

    std::cout << "Performing number of atoms experiment:\n"
        << "----------------------------------------------------------------------------\n";
    run_num_atoms_experiments();
    std::cout << "\n";

    std::cout << "Performing number of phases experiment:\n"
        << "----------------------------------------------------------------------------\n";
    run_num_phases_experiments();
    std::cout << "\n";

    return 0;
}