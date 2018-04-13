#include <boost/range/combine.hpp>
#include "plan.h"

void Plan::assign_coords(const Partition &partition, const std::vector<coord> &coords) {
    RUNTIME_ASSERT(partition.size() == coords.size());

    const Atom* atom;
    coord c;
    for (const auto &entry : boost::combine(partition, coords)) {
        boost::tie(atom, c) = entry;
        
        auto iter = m_board.find(atom);
        RUNTIME_ASSERT(iter != m_board.end());

        iter->second = c;
    }
}

void Plan::recursive_partition(bool split_horizontally) {
    auto old_partitions = std::move(m_partitions);
    auto old_bounds = std::move(m_partition_bounds);
    
    m_partitions.reserve(old_partitions.size() * 2);
    m_partition_bounds.reserve(old_bounds.size() * 2);

    auto sort_by_coord = [&](const Atom* lhs, const Atom* rhs) {
        const coord &lhs_c = m_board.at(lhs);
        const coord &rhs_c = m_board.at(rhs);
        return split_horizontally ? coord::x_major_lt(lhs_c, rhs_c) :
                                    coord::y_major_lt(lhs_c, rhs_c);
    };

    for (auto entry : boost::combine(old_partitions, old_bounds)) {
        Partition &partition = boost::get<0>(entry);
        const plan_region &region = boost::get<1>(entry);
        
        if (partition.size() == 0) {
            m_partition_bounds.emplace_back(region);
            m_partitions.emplace_back(std::move(partition));
        }
        else {
            auto mid_iter = partition.begin() + partition.size() / 2;
            std::partial_sort(partition.begin(), mid_iter, partition.end(), sort_by_coord);

            if (split_horizontally) {
                m_partition_bounds.emplace_back(bound{ region.first.begin, get_coord(**mid_iter).x }, region.second);
                m_partition_bounds.emplace_back(bound{ get_coord(**mid_iter).x, region.first.end }, region.second);
            }
            else {
                m_partition_bounds.emplace_back(region.first, bound{ region.second.begin, get_coord(**mid_iter).y });
                m_partition_bounds.emplace_back(region.first, bound{ get_coord(**mid_iter).y, region.second.end });
            }
            
            m_partitions.emplace_back(mid_iter, partition.end());
            partition.erase(mid_iter, partition.end());
            m_partitions.emplace_back(std::move(partition));
        }
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
        m_board.emplace(&atom, coord{ 0.0, 0.0 });
    };
    
    m_partitions.emplace_back(std::move(initial_partiton));
    m_partition_bounds.emplace_back(bound{ 0.0, static_cast<double>(m_height) }, 
                                    bound{ 0.0, static_cast<double>(m_width) });

    std::for_each(m_netlist.begin_luts(), m_netlist.end_luts(), insert_initial_coord);
    std::for_each(m_netlist.begin_ffs(), m_netlist.end_ffs(), insert_initial_coord);
    RUNTIME_ASSERT(m_board.size() == m_netlist.num_luts() + m_netlist.num_ffs());
}