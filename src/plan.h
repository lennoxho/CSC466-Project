#pragma once

#include <boost/optional.hpp>
#include <boost/bimap.hpp>
#include "netlist.h"

class Plan {

public:

    using Partition = std::vector<const Atom*>;

    struct coord {
        inline bool operator<(const coord &rhs) const {
            return x < rhs.x || (x == rhs.x && y < rhs.y);
        }

        static inline bool x_major_lt(const coord &lhs, const coord &rhs) {
            return lhs < rhs;
        }

        static inline bool y_major_lt(const coord &lhs, const coord &rhs) {
            return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
        }

        double x;
        double y;
    };

    Plan(std::size_t width, std::size_t height, const Netlist &netlist)
        :m_width{ width },
        m_height{ height },
        m_netlist{ netlist }
    {
        RUNTIME_ASSERT(width * height >= 2 * std::max(netlist.num_ffs(), netlist.num_luts()));
        RUNTIME_ASSERT(height >= netlist.num_ipins());
        RUNTIME_ASSERT(height >= netlist.num_opins());
    }

    Plan(const Plan&) = delete;
    Plan &operator=(const Plan&) = delete;

    const std::size_t get_width() const { return m_width; }
    const std::size_t get_height() const { return m_height; }

    inline const Netlist &get_netlist() const { return m_netlist; }

    inline auto &partitions() { return m_partitions; }
    inline auto &partitions() const { return m_partitions; }

    inline auto begin_partitions() { return m_partitions.begin(); }
    inline auto begin_partitions() const { return m_partitions.begin(); }
    inline auto end_partitions() { return m_partitions.end(); }
    inline auto end_partitions() const { return m_partitions.end(); }

    inline auto &board() { return m_board.right; }
    inline auto &board() const { return m_board.right; }

    inline auto begin_board() { return board().begin(); }
    inline auto begin_board() const { return board().begin(); }
    inline auto end_board() { return board().end(); }
    inline auto end_board() const { return board().end(); }

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

    inline coord get_coord(const Atom &atom) const {
        auto iter = m_board.right.find(&atom);
        RUNTIME_ASSERT(iter != m_board.right.end());
        return iter->second;
    }

    inline boost::optional<coord> get_coord(const IPin &ipin) const {
        auto iter = std::find_if(ipins().begin(), ipins().end(), 
            [&](const IPin &i) { return &i == &ipin; });
        if (iter == ipins().end()) return boost::none;
        return coord{ -1.0, static_cast<double>((iter - ipins().begin()) * (m_height / m_netlist.num_ipins())) };
    }

    inline boost::optional<coord> get_coord(const OPin &opin) const {
        auto iter = std::find_if(opins().begin(), opins().end(), 
            [&](const OPin &o) { return &o == &opin; });
        if (iter == opins().end()) return boost::none;
        return coord{ static_cast<double>(m_width), static_cast<double>((iter - opins().begin()) * (m_height / m_netlist.num_opins())) };
    }

    void assign_coords(const Partition &partition, const std::vector<coord> &coords);
    void recursive_partition();

private:

    void initial_setup();

    std::size_t m_width;
    std::size_t m_height;
    const Netlist &m_netlist;
    std::vector<Partition> m_partitions;
    boost::bimap<coord, const Atom*> m_board;

};