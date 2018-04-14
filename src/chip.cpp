#include <numeric>
#include <iostream>
#include "chip.h"

std::size_t Chip::swap(const Atom &lhs_atom, std::size_t idx) {
    auto lhs_iter = m_board.right.find(&lhs_atom);
    RUNTIME_ASSERT(lhs_iter != m_board.right.end());

    std::size_t lhs_ori_idx = lhs_iter->second;
    std::size_t rhs_ori_idx = lhs_atom.get_type() == Atom::type::LUT ? lut_to_idx(idx) : ff_to_idx(idx);
    RUNTIME_ASSERT(rhs_ori_idx < m_width * m_height);
    if (lhs_ori_idx == rhs_ori_idx) return idx;

    m_bbox -= bbox_for_atom(lhs_atom);

    auto rhs_iter = m_board.left.find(rhs_ori_idx);
    if (rhs_iter != m_board.left.end()) {
        const Atom &rhs_atom = *rhs_iter->second;
        m_bbox -= bbox_for_atom(rhs_atom);

        m_board.right.erase(lhs_iter);
        m_board.left.erase(rhs_iter);
        m_board.insert({ lhs_ori_idx,  &rhs_atom });
        m_board.insert({ rhs_ori_idx,  &lhs_atom });

        m_bbox += bbox_for_atom(rhs_atom);
        m_bbox += bbox_for_atom(lhs_atom);
    }
    else {
        m_board.right.erase(lhs_iter);
        m_board.insert({ rhs_ori_idx,  &lhs_atom });
        m_bbox += bbox_for_atom(lhs_atom);
    }

    return lhs_atom.get_type() == Atom::type::LUT ? lhs_ori_idx / 2 : (lhs_ori_idx - 1) / 2;
}

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
}

std::int64_t Chip::initial_bbox() {
    std::int64_t total = std::accumulate(m_netlist.begin_ipins(), m_netlist.end_ipins(), std::int64_t(0),
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
    if (auto ipin_opt = get_coord(static_cast<const IPin&>(net.get_atom()))) {
        src_coord = *ipin_opt;
    }
    else {
        src_coord = get_coord(net.get_atom());
    }

    std::int64_t min_x, max_x, min_y, max_y;
    min_x = max_x = src_coord.x;
    min_y = max_y = src_coord.y;

    for (const IPort* iport : net) {
        coord dest_coord;
        if (auto opin_opt = get_coord(static_cast<const OPin&>(iport->get_atom()))) {
            dest_coord = *opin_opt;
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

void Chip::legalize_plan(const Plan &plan) {
    for (const auto &entry : plan.board()) {
        coord new_coord{ static_cast<std::int64_t>(entry.second.x),
                         static_cast<std::int64_t>(entry.second.y) };
        new_coord.x = std::max(std::int64_t(0), new_coord.x);
        new_coord.x = std::min(static_cast<std::int64_t>(m_height)-2, new_coord.x);
        new_coord.y = std::max(std::int64_t(0), new_coord.y);
        new_coord.y = std::min(static_cast<std::int64_t>(m_width)-2, new_coord.y);
        std::size_t ori_idx = coord_to_idx(new_coord);
        std::size_t idx = ori_idx;

        if ((entry.first->get_type() == Atom::type::LUT && idx % 2 == 1) ||
            (entry.first->get_type() == Atom::type::FF && idx % 2 == 0))
        {
            ++idx;
        }

        std::size_t max_idx = m_width * m_height;

        std::int64_t curr_idx = static_cast<std::int64_t>(idx);
        auto iter = m_board.left.find(static_cast<std::size_t>(curr_idx));

        while (iter != m_board.left.end() && curr_idx - 2 > 0) {
            curr_idx -= 2;
            iter = m_board.left.find(static_cast<std::size_t>(curr_idx));
        }

        if (iter != m_board.left.end()) curr_idx = static_cast<std::int64_t>(idx);
        while (iter != m_board.left.end() && curr_idx + 2 < static_cast<std::int64_t>(max_idx)) {
            curr_idx += 2;
            iter = m_board.left.find(static_cast<std::size_t>(curr_idx));
        }

        idx = static_cast<std::size_t>(curr_idx);
        RUNTIME_ASSERT(idx < max_idx);
        m_board.insert({ idx, entry.first });
    }
}