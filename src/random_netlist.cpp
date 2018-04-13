#include <random>
#include <limits>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "netlist.h"

namespace Utils {

    Netlist random_netlist(std::size_t num_ipins, std::size_t num_opins, std::size_t num_luts,
                           std::size_t num_ffs, std::size_t num_inputs, std::size_t num_outputs)
    {
        Netlist netlist{ num_ipins, num_opins, num_luts, num_ffs, num_inputs, num_outputs, 10000000 };

        static constexpr double connect_prob = 0.25;
        std::mt19937 eng;
        std::uniform_real_distribution<> connect_dist{ 0.0, 1.0 };
        std::uniform_int_distribution<std::size_t> atom_dist{ 0, num_luts + num_ffs - 1 };
        std::uniform_int_distribution<std::size_t> oport_dist{ 0, num_outputs + num_ipins - 1 };

        auto get_atom = [&](std::size_t idx) -> Atom& {
            if (idx < num_luts) {
                return get<Netlist::LUT>(netlist, idx);
            }
            else {
                return get<Netlist::FF>(netlist, idx - num_luts);
            }
        };

        auto get_oport = [&](Atom &atom, std::size_t idx) -> OPort& {
            if (idx < num_outputs) {
                return atom.get_oport(idx);
            }
            else {
                return get<IPin>(netlist, idx - num_outputs).get_oport();
            }
        };

        for (OPin &opin : netlist.opins()) {
            if (connect_dist(eng) < connect_prob) {
                Atom &lucky_atom = get_atom(atom_dist(eng));
                OPort &lucky_oport = get_oport(lucky_atom, oport_dist(eng));

                IPort &iport = opin.get_iport();
                connect(iport, lucky_oport);
            }
        }

        for (Atom &lut : netlist.luts()) {
            for (std::size_t i = 0; i < num_inputs; ++i) {
                if (connect_dist(eng) < connect_prob) {
                    Atom &lucky_atom = get_atom(atom_dist(eng));
                    OPort &lucky_oport = get_oport(lucky_atom, oport_dist(eng));

                    IPort &iport = lut.get_iport(i);
                    connect(iport, lucky_oport);
                }
            }
        }

        for (Atom &ff : netlist.ffs()) {
            for (std::size_t i = 0; i < num_inputs; ++i) {
                if (connect_dist(eng) < connect_prob) {
                    Atom &lucky_atom = get_atom(atom_dist(eng));
                    OPort &lucky_oport = get_oport(lucky_atom, oport_dist(eng));

                    IPort &iport = ff.get_iport(i);
                    connect(iport, lucky_oport);
                }
            }
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