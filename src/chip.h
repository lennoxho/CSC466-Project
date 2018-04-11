#pragma once

#include <boost/bimap.hpp>
#include "netlist.h"

using Net = OPort;

class Chip {

public:

    struct coord {
        std::int64_t x;
        std::int64_t y;
    };

    Chip(std::size_t width, std::size_t height, const Netlist &netlist)
        :m_bbox{ 0 },
        m_width{ width },
        m_height{ height },
        m_netlist{ netlist }
    {
        RUNTIME_ASSERT(width * height >= 2 * std::max(netlist.num_ffs(), netlist.num_luts()));
        RUNTIME_ASSERT(height >= netlist.num_ipins());
        RUNTIME_ASSERT(height >= netlist.num_opins());
        initial_random_placement();
        m_bbox = initial_bbox();
    }

    Chip(const Chip&) = delete;
    Chip operator=(const Chip&) = delete;

    const std::size_t get_width() const { return m_width; }
    const std::size_t get_height() const { return m_height; }

    inline std::int64_t get_bbox() const { return m_bbox; }
    inline const Netlist &get_netlist() const { return m_netlist; }

    inline coord get_coord(const Atom &atom) const {
        auto iter = m_board.right.find(&atom);
        RUNTIME_ASSERT(iter != m_board.right.end());
        return idx_to_coord(iter->second);
    }

    std::size_t swap(const Atom &lhs_atom, std::size_t idx) {
        auto lhs_iter = m_board.right.find(&lhs_atom);
        RUNTIME_ASSERT(lhs_iter != m_board.right.end());

        std::size_t lhs_ori_idx = lhs_iter->second;
        std::size_t rhs_ori_idx = (lhs_ori_idx % 2 == 0) ? lut_to_idx(idx) : ff_to_idx(idx);
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

        return (lhs_ori_idx % 2 == 0) ? lhs_ori_idx / 2 : (lhs_ori_idx -1 ) / 2;
    }

private:

    void initial_random_placement();
    std::int64_t initial_bbox();
    std::int64_t bbox_for_net(const Net &net) const;
    std::int64_t bbox_for_atom(const Atom &atom) const;

    static inline std::size_t lut_to_idx(std::size_t lut_idx) {
        return lut_idx * 2;
    }

    static inline std::size_t ff_to_idx(std::size_t ff_idx) {
        return ff_idx * 2 + 1;
    }

    inline coord ipin_to_coord(std::size_t idx) const {
        return { -1, static_cast<std::int64_t>(idx * (m_height / m_netlist.num_ipins())) };
    }

    inline coord opin_to_coord(std::size_t idx) const {
        return { static_cast<std::int64_t>(m_width), static_cast<std::int64_t>(idx * (m_height / m_netlist.num_opins())) };
    }

    inline coord idx_to_coord(std::size_t idx) const {
        return { static_cast<std::int64_t>(idx / m_width), static_cast<std::int64_t>(idx % m_width) };
    }

    inline std::size_t coord_to_idx(const coord &c) const {
        return c.x * m_width + c.y;
    }

    std::int64_t m_bbox;
    std::size_t m_width;
    std::size_t m_height;
    const Netlist &m_netlist;
    boost::bimap<std::size_t, const Atom*> m_board;
    boost::bimap<std::size_t, const Atom*> m_ipins;
    boost::bimap<std::size_t, const Atom*> m_opins;

};