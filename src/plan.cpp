#include <boost/range/combine.hpp>
#include "plan.h"

void Plan::assign_coords(const Partition &partition, const std::vector<coord> &coords) {
    RUNTIME_ASSERT(partition.size() == coords.size());

    const Atom* atom;
    coord c;
    for (const auto &entry : boost::combine(partition, coords)) {
        boost::tie(atom, c) = entry;
        
        auto iter = m_board.right.find(atom);
        RUNTIME_ASSERT(iter != m_board.right.end());

        m_board.right.erase(iter);
        m_board.insert({ c, atom });
    }
}

void Plan::recursive_partition() {
    auto old_partitions = std::move(m_partitions);
    RUNTIME_ASSERT(m_partitions.empty());
    m_partitions.reserve(old_partitions.size() * 2);

    const bool split_horizontally = true;
    auto sort_by_coord = [&](const Atom* lhs, const Atom* rhs) {
        const coord &lhs_c = m_board.right.at(lhs);
        const coord &rhs_c = m_board.right.at(rhs);
        return split_horizontally ? coord::x_major_lt(lhs_c, rhs_c) :
                                    coord::y_major_lt(lhs_c, rhs_c);
    };

    for (auto &partition : old_partitions) {
        auto mid_iter = partition.begin() + partition.size() / 2;
        std::partial_sort(partition.begin(), mid_iter, partition.end(), sort_by_coord);

        m_partitions.emplace_back(mid_iter, partition.end());
        partition.erase(mid_iter, partition.end());
        m_partitions.emplace_back(std::move(partition));
    }
}

void Plan::initial_setup() {
    Partition initial_partiton;
    initial_partiton.resize(m_netlist.num_luts() + m_netlist.num_ffs());
    auto iter = std::transform(m_netlist.begin_luts(), m_netlist.end_luts(), initial_partiton.begin(),
        [](const Atom &lut) { return &lut; });
    std::transform(m_netlist.begin_ffs(), m_netlist.end_ffs(), iter,
        [](const Atom &ff) { return &ff; });

    auto insert_initial_coord = [&](const Atom &atom) {
        m_board.insert({ { 0.0, 0.0 }, &atom });
    };

    std::for_each(m_netlist.begin_luts(), m_netlist.end_luts(), insert_initial_coord);
    std::for_each(m_netlist.begin_ffs(), m_netlist.end_ffs(), insert_initial_coord);
    RUNTIME_ASSERT(m_board.size() == m_netlist.num_luts() + m_netlist.num_ffs());
}