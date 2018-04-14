#pragma once

#include <boost/range/adaptors.hpp>
#include "plan.h"

using Net = OPort;

class Chip {

public:

    struct coord {
        std::int64_t x;
        std::int64_t y;
    };

    Chip(std::size_t width, std::size_t height, const Netlist &netlist)
        :m_width{ width },
        m_height{ height },
        m_netlist{ netlist }
    {
        RUNTIME_ASSERT(width * height >= 2 * std::max(netlist.num_ffs(), netlist.num_luts()));
        RUNTIME_ASSERT(height >= netlist.num_ipins());
        RUNTIME_ASSERT(height >= netlist.num_opins());
        initial_random_placement();
        m_bbox = initial_bbox();
    }

    Chip(const Plan &plan)
        :m_width{ plan.get_width() },
        m_height{ plan.get_height() },
        m_netlist{ plan.get_netlist() }
    {
        legalize_plan(plan);
        m_bbox = initial_bbox();
    }

    Chip(Chip &&other)
        :m_bbox{ other.m_bbox },
        m_width{ other.m_width },
        m_height{ other.m_height },
        m_netlist{ other.m_netlist },
        m_board{ std::move(other.m_board) }
    {}

    Chip operator=(const Chip&) = delete;

    const std::size_t get_width() const { return m_width; }
    const std::size_t get_height() const { return m_height; }

    inline std::int64_t get_bbox() const { return m_bbox; }
    inline const Netlist &get_netlist() const { return m_netlist; }

    inline auto ipins() { return m_netlist.ipins(); }
    inline auto ipins() const { return m_netlist.ipins(); }

    inline auto begin_ipins() { return ipins().begin(); }
    inline auto begin_ipins() const { return ipins().begin(); }
    inline auto end_ipins() { return ipins().end(); }
    inline auto end_ipins() const { return ipins().end(); }

    inline auto opins() { return m_netlist.opins(); }
    inline auto opins() const { return m_netlist.opins(); }

    inline auto begin_opins() { return opins().begin(); }
    inline auto begin_opins() const { return opins().begin(); }
    inline auto end_opins() { return opins().end(); }
    inline auto end_opins() const { return opins().end(); }

    inline auto coords() const {
        return m_board.left
            | boost::adaptors::transformed([&](const auto &entry) { return idx_to_coord(entry.first); });
    }

    inline coord get_coord(const Atom &atom) const {
        auto iter = m_board.right.find(&atom);
        RUNTIME_ASSERT(iter != m_board.right.end());
        return idx_to_coord(iter->second);
    }

    inline boost::optional<coord> get_coord(const IPin &ipin) const {
        auto iter = std::find_if(ipins().begin(), ipins().end(),
            [&](const IPin &i) { return &i == &ipin; });
        if (iter == ipins().end()) return boost::none;
        return coord{ -1, static_cast<std::int64_t>((iter - ipins().begin()) * (m_height / m_netlist.num_ipins())) };
    }

    inline boost::optional<coord> get_coord(const OPin &opin) const {
        auto iter = std::find_if(opins().begin(), opins().end(),
            [&](const OPin &o) { return &o == &opin; });
        if (iter == opins().end()) return boost::none;
        return coord{ static_cast<std::int64_t>(m_width), static_cast<std::int64_t>((iter - opins().begin()) * (m_height / m_netlist.num_opins())) };
    }

    std::size_t swap(const Atom &lhs_atom, std::size_t idx);

private:

    void initial_random_placement();
    std::int64_t initial_bbox();
    std::int64_t bbox_for_net(const Net &net) const;
    std::int64_t bbox_for_atom(const Atom &atom) const;

    void legalize_plan(const Plan &plan);

    static inline std::size_t lut_to_idx(std::size_t lut_idx) {
        return lut_idx * 2;
    }

    static inline std::size_t ff_to_idx(std::size_t ff_idx) {
        return ff_idx * 2 + 1;
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

};