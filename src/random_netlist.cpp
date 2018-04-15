#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <random>

#include "netlist.h"

namespace Utils {

    namespace impl {

        void random_front_phase(Netlist &netlist, const std::vector<Atom*> &phase, std::mt19937 &eng,
            std::size_t num_outputs, std::size_t num_ipins, double connect_prob) {
            std::size_t atom_skew = phase.size() / 15;

            std::uniform_real_distribution<> connect_dist{ 0.0, 1.0 };
            std::uniform_int_distribution<std::size_t> atom_dist{ 0, phase.size() - 1 };
            std::uniform_int_distribution<std::size_t> oport_dist{ 0, num_outputs*atom_skew + num_ipins - 1 };
            std::uniform_int_distribution<std::size_t> ipin_dist{ 0, num_ipins - 1 };

            auto get_oport = [&](Atom &atom, std::size_t idx) -> OPort& {
                std::size_t real_idx = idx % num_outputs;
                if (idx / num_outputs < atom_skew) {
                    return atom.get_oport(real_idx);
                }
                else {
                    return get<IPin>(netlist, ipin_dist(eng)).get_oport();
                }
            };

            for (Atom* atom : phase) {
                for (IPort &iport : atom->inputs()) {
                    if (!iport.has_fanin() && connect_dist(eng) < connect_prob) {
                        Atom &lucky_atom = *phase[atom_dist(eng)];
                        OPort &lucky_oport = get_oport(lucky_atom, oport_dist(eng));

                        connect(iport, lucky_oport);
                    }
                }
            }
        }

        void random_back_phase(Netlist &netlist, const std::vector<Atom*> &phase, std::mt19937 &eng,
            std::size_t num_outputs, std::size_t num_ipins, double connect_prob, bool skip_atom)
        {
            std::uniform_real_distribution<> connect_dist{ 0.0, 1.0 };
            std::uniform_int_distribution<std::size_t> atom_dist{ 0, phase.size() - 1 };
            std::uniform_int_distribution<std::size_t> oport_dist{ 0, num_outputs - 1 };
            std::uniform_int_distribution<std::size_t> ipin_dist{ 0, num_ipins - 1 };

            for (OPin &opin : netlist.opins()) {
                Atom &lucky_atom = *phase[atom_dist(eng)];
                OPort &lucky_oport = lucky_atom.get_oport(oport_dist(eng));

                IPort &iport = opin.get_iport();
                connect(iport, lucky_oport);
            }

            if (!skip_atom) {
                for (Atom* atom : phase) {
                    for (IPort &iport : atom->inputs()) {
                        if (!iport.has_fanin() && connect_dist(eng) < connect_prob) {
                            Atom &lucky_atom = *phase[atom_dist(eng)];
                            OPort &lucky_oport = lucky_atom.get_oport(oport_dist(eng));

                            connect(iport, lucky_oport);
                        }
                    }
                }
            }
        }

        void random_phase(const std::vector<Atom*> &phase, std::mt19937 &eng,
            std::size_t num_outputs, std::size_t num_ipins, double connect_prob)
        {
            std::uniform_real_distribution<> connect_dist{ 0.0, 1.0 };
            std::uniform_int_distribution<std::size_t> atom_dist{ 0, phase.size() - 1 };
            std::uniform_int_distribution<std::size_t> oport_dist{ 0, num_outputs - 1 };
            std::uniform_int_distribution<std::size_t> ipin_dist{ 0, num_ipins - 1 };

            for (Atom* atom : phase) {
                for (IPort &iport : atom->inputs()) {
                    if (!iport.has_fanin() && connect_dist(eng) < connect_prob) {
                        Atom &lucky_atom = *phase[atom_dist(eng)];
                        OPort &lucky_oport = lucky_atom.get_oport(oport_dist(eng));

                        connect(iport, lucky_oport);
                    }
                }
            }
        }

        void connect_phases(const std::vector<std::vector<Atom*>> &phases, std::size_t num_connections, std::mt19937 &eng, std::size_t num_inputs, std::size_t num_outputs) {
            std::uniform_int_distribution<std::size_t> iport_dist{ 0, num_inputs - 1 };
            std::uniform_int_distribution<std::size_t> oport_dist{ 0, num_outputs - 1 };
            std::vector<std::uniform_int_distribution<std::size_t>> atom_dist;
            std::transform(phases.begin(), phases.end(), std::back_inserter(atom_dist),
                [](const auto &phase) { return std::uniform_int_distribution<std::size_t>{ 0, phase.size() - 1 }; });

            std::vector<std::size_t> indices(phases.size());
            std::iota(indices.begin(), indices.end(), 0);

            for (std::size_t i = 0; i < num_connections; ++i) {
                std::shuffle(indices.begin(), indices.end(), eng);

                std::size_t lhs_idx = atom_dist[indices[0]](eng);
                Atom &lhs_atom = *(phases[indices[0]][lhs_idx]);
                IPort &lhs_iport = lhs_atom.get_iport(iport_dist(eng));

                std::size_t rhs_idx = atom_dist[indices[1]](eng);
                Atom &rhs_atom = *(phases[indices[1]][rhs_idx]);
                OPort &rhs_oport = rhs_atom.get_oport(oport_dist(eng));

                if (!lhs_iport.has_fanin()) {
                    connect(lhs_iport, rhs_oport);
                }
            }
        }

    }

    Netlist random_netlist(std::size_t num_ipins, std::size_t num_opins, std::size_t num_luts,
        std::size_t num_ffs, std::size_t num_inputs, std::size_t num_outputs, std::size_t num_phases)
    {
        RUNTIME_ASSERT(num_ipins > 0);
        RUNTIME_ASSERT(num_opins > 0);
        RUNTIME_ASSERT(num_luts > 0);
        RUNTIME_ASSERT(num_ffs > 0);
        RUNTIME_ASSERT(num_inputs > 0);
        RUNTIME_ASSERT(num_outputs > 0);
        RUNTIME_ASSERT(num_phases > 0);

        constexpr double connect_prob = 0.25;
        constexpr std::size_t max_outputs = 10000;

        std::size_t num_luts_per_phase = num_luts / num_phases;
        std::size_t num_ffs_per_phase = num_ffs / num_phases;
        Netlist netlist{ num_ipins, num_opins, num_luts, num_ffs, num_inputs, num_outputs, max_outputs };

        std::vector<std::vector<Atom*>> phases;
        auto lut_iter = netlist.begin_luts();
        auto ff_iter = netlist.begin_ffs();

        for (std::size_t i = 0; i < num_phases - 1; ++i) {
            std::vector<Atom*> phase;
            phase.reserve(num_luts_per_phase + num_ffs_per_phase);
            std::transform(lut_iter, lut_iter + num_luts_per_phase, std::back_inserter(phase),
                [](Atom &atom) { return &atom; });
            std::transform(ff_iter, ff_iter + num_ffs_per_phase, std::back_inserter(phase),
                [](Atom &atom) { return &atom; });
            for (Atom* atom : phase) atom->set_phase(i);

            phases.emplace_back(std::move(phase));

            lut_iter += num_luts_per_phase;
            ff_iter += num_ffs_per_phase;
        }
        {
            std::vector<Atom*> phase;
            phase.reserve(num_ffs + num_luts - (num_luts_per_phase + num_ffs_per_phase) * (num_phases - 1));
            std::transform(lut_iter, netlist.end_luts(), std::back_inserter(phase),
                [](Atom &atom) { return &atom; });
            std::transform(ff_iter, netlist.end_ffs(), std::back_inserter(phase),
                [](Atom &atom) { return &atom; });
            for (Atom* atom : phase) atom->set_phase(num_phases - 1);

            phases.emplace_back(std::move(phase));
        }

        std::mt19937 eng;

        if (phases.size() > 1) {
            impl::connect_phases(phases, num_phases * (num_phases - 1) / 2 * 10, eng, num_inputs, num_outputs);
        }

        impl::random_front_phase(netlist, phases.front(), eng, num_outputs, num_ipins, connect_prob);
        impl::random_back_phase(netlist, phases.back(), eng, num_outputs, num_ipins, connect_prob, phases.size() == 1);
        for (std::size_t i = 1; i < num_phases - 1; ++i) {
            impl::random_phase(phases[i], eng, num_outputs, num_ipins, connect_prob);
        }

        return netlist;
    }

    namespace impl {

        template <typename T>
        auto ptr_str(const T &val) {
            return std::to_string(reinterpret_cast<std::intptr_t>(&val));
        };

        template <typename T>
        auto unnamed_node(const T &val) {
            boost::property_tree::ptree node;
            node.put("", ptr_str(val));
            return node;
        };

        auto expand_port(const OPort &oport) {
            boost::property_tree::ptree node;
            for (const IPort* iport : boost::make_iterator_range(oport.begin(), oport.end())) {
                node.push_back(std::make_pair("", unnamed_node(*iport)));
            }
            return node;
        }

        auto expand_port(const IPort &iport) {
            boost::property_tree::ptree node;
            if (iport.has_fanin()) {
                node.push_back(std::make_pair("", unnamed_node(*(iport.fanin()))));
            }
            return node;
        }
    }

    void dump_netlist(const Netlist &netlist, const std::string &filepath) {
        boost::property_tree::ptree tree;

        auto port_nodes = [&](const auto &ports) {
            boost::property_tree::ptree node;
            for (const auto &port : ports) {
                node.add_child(impl::ptr_str(port), impl::expand_port(port));
            }
            return node;
        };

        auto add_atoms = [&](const std::string &name, const auto &atoms) {
            boost::property_tree::ptree atoms_arr;
            for (const auto &atom : atoms) {
                boost::property_tree::ptree atom_node;
                atom_node.add_child("iports", port_nodes(atom.inputs()));
                atom_node.add_child("oports", port_nodes(atom.outputs()));

                boost::property_tree::ptree phase_node;
                phase_node.put("", atom.get_phase());
                atom_node.push_back(std::make_pair("phase", phase_node));

                atoms_arr.add_child(impl::ptr_str(atom), atom_node);
            }
            tree.add_child(name, atoms_arr);
        };

        add_atoms("IPins", netlist.ipins());
        add_atoms("OPins", netlist.opins());
        add_atoms("LUTs", netlist.luts());
        add_atoms("FFs", netlist.ffs());

        std::ofstream hfile{ filepath, std::ios::binary };
        RUNTIME_ASSERT(hfile.is_open());

        boost::property_tree::write_json(hfile, tree);
        hfile.flush();
        RUNTIME_ASSERT(hfile);
    }

}