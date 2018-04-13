#pragma once

#include <fstream>
#include "chip.h"

namespace Utils {

    class metric_consumer {

    public:

        metric_consumer(const std::string &iter_filename, const std::string &snapshot_filename)
            :m_iteration_stream{ std::make_unique<std::ofstream>(iter_filename, std::ios::out | std::ios::binary) },
            m_snapshot_stream{ std::make_unique<std::ofstream>(snapshot_filename, std::ios::out | std::ios::binary) }
        {
            RUNTIME_ASSERT(m_iteration_stream && *m_iteration_stream);
            RUNTIME_ASSERT(m_snapshot_stream && *m_snapshot_stream);
        }

        metric_consumer &operator=(const metric_consumer&) = delete;
        metric_consumer(const metric_consumer&) = delete;

        inline std::ostream &iter() { return *m_iteration_stream; }
        inline std::ostream &snapshot() { return *m_snapshot_stream; }

        inline operator bool() const {
            m_iteration_stream->flush();
            m_snapshot_stream->flush();
            return bool(*m_iteration_stream) && bool(*m_snapshot_stream);
        }

    private:

        std::unique_ptr<std::ostream> m_iteration_stream;
        std::unique_ptr<std::ostream> m_snapshot_stream;

    };

    void random_placement(Chip &chip, std::int64_t num_iter, metric_consumer* met = nullptr);
    void simulated_annealing(Chip &chip, std::int64_t num_iter, std::size_t num_swap_per_atom, double hot, double cooling_factor, metric_consumer* met = nullptr);

    void dump_plan(const Plan &plan, std::ostream &os);
    Plan quadratic_placement(std::size_t width, std::size_t height, const Netlist &netlist, int num_iter, std::size_t expected_phases, metric_consumer* met = nullptr);

}