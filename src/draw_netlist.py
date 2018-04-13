from __future__ import print_function
import json
import networkx as nx
import matplotlib.pyplot as plt
import sys

FFs = "FFs"
OPins = "OPins"
IPins = "IPins"
LUTs = "LUTs"

def load_netlist(filepath):
    try:
        hfile = open(filepath, "rb")
    except Exception as e:
        sys.stderr.write("Failed to load \"%s\"\n" % filepath)
        exit(1)

    data = json.load(hfile)
    hfile.close()
    return data

def get_iport_to_atom_relation(netlist):
    relation = dict()

    def add_relations_for_atoms(atoms):
        for atom_name in atoms.keys():
            atom = atoms[atom_name]
            iports = atom['iports']

            if iports != u'':
                for iport_name in iports.keys():
                    relation[iport_name] = atom_name

    add_relations_for_atoms(netlist[FFs])
    add_relations_for_atoms(netlist[OPins])
    add_relations_for_atoms(netlist[IPins])
    add_relations_for_atoms(netlist[LUTs])

    return relation

def add_edges_for_atoms(network, atoms, iport_to_atom):
    for atom_name in atoms.keys():
        atom = atoms[atom_name]
        oports = atom['oports']

        if oports != u'':
            for oport in oports.keys():
                iports = oports[oport]
                if iports != u'':
                    for iport in iports:
                        network.add_edge(atom_name, iport_to_atom[iport])

def get_fixed_pos(nodes, x, min_y, max_y):
    y_range = max_y - min_y
    y_distance = y_range / len(nodes)

    result = dict()
    for i in range(len(nodes)):
        node = nodes[i]
        result[node] = (x, min_y + i*y_distance)

    return result

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write("USAGE:  python draw_netlist.py <filepath>\n")
        exit(1)

    json_filepath = sys.argv[1]
    netlist = load_netlist(json_filepath)
    assert netlist is not None

    iport_to_atom = get_iport_to_atom_relation(netlist)

    network = nx.DiGraph()
    network.add_nodes_from(netlist[FFs].keys(), atom_type="FF")
    network.add_nodes_from(netlist[OPins].keys(), atom_type="OPin")
    network.add_nodes_from(netlist[IPins].keys(), atom_type="IPin")
    network.add_nodes_from(netlist[LUTs].keys(), atom_type="LUT")

    add_edges_for_atoms(network, netlist[FFs], iport_to_atom)
    add_edges_for_atoms(network, netlist[OPins], iport_to_atom)
    add_edges_for_atoms(network, netlist[IPins], iport_to_atom)
    add_edges_for_atoms(network, netlist[LUTs], iport_to_atom)

    pos = nx.spring_layout(network, k=0.5, iterations=1)
    min_x = min(coord[0] for coord in pos.values())
    max_x = max(coord[0] for coord in pos.values())
    min_y = min(coord[1] for coord in pos.values())
    max_y = max(coord[1] for coord in pos.values())

    IPins = [x[0] for x in network.nodes(data=True) if x[1]["atom_type"] == "IPin"]
    IPin_pos = get_fixed_pos(IPins, min_x - 0.1*(max_x - min_x), min_y, max_y)
    OPins = [x[0] for x in network.nodes(data=True) if x[1]["atom_type"] == "OPin"]
    OPin_pos = get_fixed_pos(OPins, max_x + 0.1*(max_x - min_x), min_y, max_y)

    for node in IPins:
        pos[node] = IPin_pos[node]
    for node in OPins:
        pos[node] = OPin_pos[node]

    nx.draw_networkx_nodes(network, pos, node_color="orange", nodelist=[x[0] for x in network.nodes(data=True) if x[1]["atom_type"] == "FF"], node_shape="s", node_size = 50)
    nx.draw_networkx_nodes(network, OPin_pos, node_color="green", nodelist=OPins, node_shape="<", node_size = 50)
    nx.draw_networkx_nodes(network, IPin_pos, node_color="blue", nodelist=IPins, node_shape=">", node_size = 50)
    nx.draw_networkx_nodes(network, pos, node_color="red", nodelist=[x[0] for x in network.nodes(data=True) if x[1]["atom_type"] == "LUT"], node_shape="s", node_size = 50)
    nx.draw_networkx_edges(network, pos, arrows=False)
    plt.show()