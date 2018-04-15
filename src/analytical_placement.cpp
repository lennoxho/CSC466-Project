// (C) Copyright Shou Hao Ho   2018
// Distributed under the MIT Software License (See accompanying LICENSE file)

#include <Eigen/Dense>
#include <boost/range/combine.hpp>
#include <boost/range/adaptors.hpp>
#include <numeric>

#include "placement.h"
#include "plan.h"

namespace Utils {

    namespace impl {

        Plan::coord get_pin_coord(const Plan &plan, const Atom &atom, const Plan::plan_region &region) {
            Plan::coord nearest_coord;

            if (auto c = plan.get_coord(static_cast<const IPin&>(atom))) {
                nearest_coord = *c;
            }
            else if (auto c = plan.get_coord(static_cast<const OPin&>(atom))) {
                nearest_coord = *c;
            }
            else {
                nearest_coord = plan.get_coord(atom);
            }

            if (nearest_coord.x < region.first.begin) {
                nearest_coord.x = region.first.begin;
            }
            else if (nearest_coord.x > region.first.end) {
                nearest_coord.x = region.first.end;
            }

            if (nearest_coord.y < region.second.begin) {
                nearest_coord.y = region.second.begin;
            }
            else if (nearest_coord.y > region.second.end) {
                nearest_coord.y = region.second.end;
            }

            return nearest_coord;
        }

    }

    void dump_plan(const Plan &plan, std::ostream &os) {
        for (const auto &entry : plan.board()) {
            os << "(" << entry.second.x << "," << entry.second.y << ")\n";
        }
    }

    Plan quadratic_placement(std::size_t width, std::size_t height, const Netlist &netlist, int num_iter,
        Plan::partitioning_method method, std::size_t expected_phases, metric_consumer* met) {
        double pin_weight_factor = 1.0 / expected_phases;

        Plan plan{ width, height, netlist };
        auto has_fanin = [](const IPort &iport) { return iport.has_fanin(); };

        std::int64_t avg_conn_per_ipin = std::accumulate(netlist.begin_ipins(), netlist.end_ipins(), 0,
            [](std::int64_t prev, const IPin &ipin) { return prev + ipin.get_oport().size(); });
        avg_conn_per_ipin /= netlist.num_ipins();

        bool split_vertically = true;
        for (int i = 0; i < num_iter; ++i) {
            if (i > 0) {
                plan.recursive_partition(split_vertically, method);
                split_vertically = !split_vertically;
            }

            std::vector<std::vector<Plan::coord>> solutions;
            for (const auto &entry : boost::combine(plan.partitions(), plan.bounds())) {
                const Plan::Partition &partition = boost::get<0>(entry);
                const Plan::plan_region &region = boost::get<1>(entry);
                if (partition.size() == 0) continue;

                std::unordered_map<const Atom*, std::size_t> atom_to_index;
                std::size_t i = 0;
                for (const Atom* atom : partition) {
                    atom_to_index[atom] = i++;
                }

                Eigen::MatrixXd A = Eigen::MatrixXd::Zero(partition.size(), partition.size());

                Eigen::VectorXd b_x = Eigen::VectorXd::Zero(partition.size());
                Eigen::VectorXd b_y = Eigen::VectorXd::Zero(partition.size());

                for (std::size_t x = 0; x < partition.size(); ++x) {
                    const Atom* atom = partition[x];

                    auto register_target = [&](double inv_weight, const Atom &target_atom) {
                        if (&target_atom == atom) return;
                        A(x, x) += inv_weight;

                        auto idx_iter = atom_to_index.find(&target_atom);
                        if (idx_iter != atom_to_index.end()) {
                            std::size_t y = idx_iter->second;
                            RUNTIME_ASSERT(x != y);
                            A(x, y) += -inv_weight;
                        }
                        else {
                            auto coord = impl::get_pin_coord(plan, target_atom, region);
                            inv_weight *= pin_weight_factor;
                            if (target_atom.get_type() == Atom::type::OPIN) inv_weight *= avg_conn_per_ipin;

                            b_x(x) += inv_weight * coord.x;
                            b_y(x) += inv_weight * coord.y;
                        }
                    };

                    for (const IPort &iport : atom->inputs() | boost::adaptors::filtered(has_fanin)) {
                        const OPort* target_oport = iport.fanin();
                        double inv_weight = 1.0 / target_oport->size();
                        const Atom &target_atom = target_oport->get_atom();

                        register_target(inv_weight, target_atom);
                    }

                    for (const OPort &oport : atom->outputs()) {
                        if (oport.empty()) continue;
                        double inv_weight = 1.0 / oport.size();

                        for (const IPort* target_iport : oport) {
                            const Atom &target_atom = target_iport->get_atom();

                            register_target(inv_weight, target_atom);
                        }
                    }
                }

                A.ldlt().solveInPlace(b_x);
                A.ldlt().solveInPlace(b_y);

                std::vector<Plan::coord> sol(partition.size());
                for (std::size_t j = 0; j < partition.size(); ++j) {
                    sol[j] = Plan::coord{ b_x(j), b_y(j) };
                }

                solutions.emplace_back(std::move(sol));
            }

            for (std::size_t i = 0; i < solutions.size(); ++i) {
                plan.assign_coords(plan.partitions()[i], solutions[i], plan.bounds()[i]);
            }

            if (met != nullptr) {
                met->snapshot() << "ss " << i << " (" << width << "," << height << "):\n";
                dump_plan(plan, met->snapshot());
            }
        }

        return plan;
    }

}