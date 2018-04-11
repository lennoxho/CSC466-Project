#include <numeric>
#include "chip.h"

void Chip::initial_random_placement() {
    std::size_t i = 0;
    for (const auto &lut : m_netlist.luts()) {
        m_board.insert({ lut_to_idx(i), &lut });
        ++i;
    }
    i = 0;
    for (const auto &ff : m_netlist.ffs()) {
        m_board.insert({ ff_to_idx(i), &ff });
        ++i;
    }
    RUNTIME_ASSERT(m_board.size() == m_netlist.num_luts() + m_netlist.num_ffs());

    i = 0;
    for (const auto &ipin : m_netlist.ipins()) {
        m_ipins.insert({ i, static_cast<const Atom*>(&ipin) });
        ++i;
    }
    RUNTIME_ASSERT(m_ipins.size() == m_netlist.num_ipins());
    i = 0;
    for (const auto &opin : m_netlist.opins()) {
        m_opins.insert({ i, static_cast<const Atom*>(&opin) });
        ++i;
    }
    RUNTIME_ASSERT(m_opins.size() == m_netlist.num_opins());
}

std::int64_t Chip::initial_bbox() {
    std::int64_t total = std::accumulate(m_netlist.begin_ipins(), m_netlist.end_ipins(), 0,
        [&](std::int64_t prev, const IPin &ipin) { return prev + bbox_for_net(ipin.get_oport()); });

    auto acc_oports = [&](const auto &range) {
        return std::accumulate(range.begin(), range.end(), 0,
            [&](std::int64_t prev, const OPort &oport) { return prev + bbox_for_net(oport); });
    };

    total += std::accumulate(m_netlist.begin_luts(), m_netlist.end_luts(), 0,
        [&](std::int64_t prev, const Atom &atom) { return prev + acc_oports(atom.outputs()); });

    total += std::accumulate(m_netlist.begin_ffs(), m_netlist.end_ffs(), 0,
        [&](std::int64_t prev, const Atom &atom) { return prev + acc_oports(atom.outputs()); });

    return total;
}

std::int64_t Chip::bbox_for_net(const Net &net) const {
    coord src_coord;
    auto iter = m_ipins.right.find(&net.get_atom());
    if (iter != m_ipins.right.end()) {
        src_coord = ipin_to_coord(iter->second);
    }
    else {
        src_coord = get_coord(net.get_atom());
    }

    std::int64_t min_x, max_x, min_y, max_y;
    min_x = max_x = src_coord.x;
    min_y = max_y = src_coord.y;

    for (const IPort* iport : net) {
        coord dest_coord;
        auto iter = m_opins.right.find(&iport->get_atom());
        if (iter != m_opins.right.end()) {
            dest_coord = opin_to_coord(iter->second);
        }
        else {
            dest_coord = get_coord(iport->get_atom());
        }
        min_x = std::min(min_x, static_cast<std::int64_t>(dest_coord.x));
        max_x = std::max(max_x, static_cast<std::int64_t>(dest_coord.x));
        min_y = std::min(min_y, static_cast<std::int64_t>(dest_coord.y));
        max_y = std::max(max_y, static_cast<std::int64_t>(dest_coord.y));
    }

    std::int64_t x_diff = max_x - min_x;
    std::int64_t y_diff = max_y - min_y;
    return std::abs(x_diff) + std::abs(y_diff);
}

std::int64_t Chip::bbox_for_atom(const Atom &atom) const {
    auto acc_iports = [&](std::int64_t prev, const IPort &iport) {
        return prev + (iport.has_fanin() ? bbox_for_net(*iport.fanin()) : 0);
    };
    std::int64_t total = std::accumulate(atom.begin_inputs(), atom.end_inputs(), 0, acc_iports);

    auto acc_oports = [&](std::int64_t prev, const OPort &oport) {
        return prev + bbox_for_net(oport);
    };
    total += std::accumulate(atom.begin_outputs(), atom.end_outputs(), 0, acc_oports);

    return total;
}