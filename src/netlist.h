#pragma once

#include <exception>
#include <vector>
#include <boost/range.hpp>

#define RUNTIME_ASSERT(COND) if (!(COND)) { throw std::runtime_error{ #COND }; }

class OPort;
class Atom;
namespace Utils {
    struct Access;
}

class IPort {

    friend class Atom;

public:

    IPort(Atom &atom)
        :m_fanin{ nullptr },
        m_atom{ &atom }
    {}

    IPort(const IPort &other)
        :m_fanin{ other.m_fanin },
        m_atom{ other.m_atom }
    {}

    IPort &operator=(const IPort &other) {
        m_fanin = other.m_fanin;
        m_atom = other.m_atom;
        return *this;
    }

    inline OPort* &fanin() { return m_fanin; }
    inline const OPort* const fanin() const { return m_fanin; }
    inline bool has_fanin() const { return m_fanin != nullptr; }

    inline Atom &get_atom() { return *m_atom; }
    inline const Atom &get_atom() const { return *m_atom; }

private:

    OPort* m_fanin;
    Atom* m_atom;

};

class OPort {

    friend class Atom;

public:

    OPort(Atom &atom, std::size_t max_fanouts)
        :m_fanouts{},
        max_fanouts{ max_fanouts },
        m_atom{ &atom }
    {}

    OPort(const OPort &other)
        :m_fanouts{ other.m_fanouts },
        max_fanouts{ other.max_fanouts },
        m_atom{ other.m_atom }
    {}

    OPort(OPort &&other)
        :m_fanouts{ std::move(other.m_fanouts) },
        max_fanouts{ other.max_fanouts },
        m_atom{ other.m_atom }
    {}

    OPort &operator=(const OPort &other) {
        m_fanouts = other.m_fanouts;
        max_fanouts = other.max_fanouts;
        m_atom = other.m_atom;
        return *this;
    }

    OPort &operator=(OPort &&other) {
        m_fanouts = std::move(other.m_fanouts);
        max_fanouts = other.max_fanouts;
        m_atom = other.m_atom;
        return *this;
    }

    inline auto begin() { return m_fanouts.begin(); }
    inline auto begin() const { return m_fanouts.begin(); }
    inline auto end() { return m_fanouts.end(); }
    inline auto end() const { return m_fanouts.end(); }

    inline bool empty() const { return m_fanouts.empty(); }
    inline std::size_t size() const { return m_fanouts.size(); }
    inline std::size_t capacity_left() const { return max_fanouts - size(); }

    IPort &push_back(IPort &iport) {
        RUNTIME_ASSERT(capacity_left() > 0);
        m_fanouts.emplace_back(&iport);
        return *m_fanouts.back();
    }

    inline Atom &get_atom() { return *m_atom; }
    inline const Atom &get_atom() const { return *m_atom; }

private:

    std::vector<IPort*> m_fanouts;
    std::size_t max_fanouts;
    Atom* m_atom;

};

class Atom {

public:

    Atom(std::size_t max_inputs, std::size_t max_outputs, std::size_t max_fanouts)
        :m_inputs( max_inputs, IPort{ *this } ),
        m_outputs( max_outputs, OPort{ *this, max_fanouts } )
    {}

    Atom(const Atom &other)
        :m_inputs{ other.m_inputs },
        m_outputs{ other.m_outputs }
    {}

    Atom(Atom &&other)
        :m_inputs{ std::move(other.m_inputs) },
        m_outputs{ std::move(other.m_outputs) }
    {}

    Atom &operator=(const Atom &other) {
        m_inputs = other.m_inputs;
        m_outputs = other.m_outputs;
        return *this;
    }

    Atom &operator=(Atom &&other) {
        m_inputs = std::move(other.m_inputs);
        m_outputs = std::move(other.m_outputs);
        return *this;
    }

    // HACK - bad design decisions lead to this :(.
    void init_back_pointer() {
        for (IPort &iport : m_inputs) {
            iport.m_atom = this;
        }
        for (OPort &oport : m_outputs) {
            oport.m_atom = this;
        }
    }

    inline auto begin_inputs() { return m_inputs.begin(); }
    inline auto begin_inputs() const { return m_inputs.begin(); }
    inline auto end_inputs() { return m_inputs.end(); }
    inline auto end_inputs() const { return m_inputs.end(); }

    inline auto begin_outputs() { return m_outputs.begin(); }
    inline auto begin_outputs() const { return m_outputs.begin(); }
    inline auto end_outputs() { return m_outputs.end(); }
    inline auto end_outputs() const { return m_outputs.end(); }

    inline auto inputs() { return boost::make_iterator_range(begin_inputs(), end_inputs()); }
    inline auto inputs() const { return boost::make_iterator_range(begin_inputs(), end_inputs()); }

    inline auto outputs() { return boost::make_iterator_range(begin_outputs(), end_outputs()); }
    inline auto outputs() const { return boost::make_iterator_range(begin_outputs(), end_outputs()); }

    inline IPort &get_iport(std::size_t idx) {
        RUNTIME_ASSERT(idx < m_inputs.size());
        return m_inputs[idx];
    }

    const inline IPort &get_iport(std::size_t idx) const {
        RUNTIME_ASSERT(idx < m_inputs.size());
        return m_inputs[idx];
    }

    inline OPort &get_oport(std::size_t idx) {
        RUNTIME_ASSERT(idx < m_outputs.size());
        return m_outputs[idx];
    }

    const inline OPort &get_oport(std::size_t idx) const {
        RUNTIME_ASSERT(idx < m_outputs.size());
        return m_outputs[idx];
    }

    inline std::size_t num_unconnected_input() const {
        return std::count_if(m_inputs.begin(), m_inputs.end(),
                             [](const IPort &iport) { return !iport.has_fanin(); });
    }

    inline std::size_t num_unconnected_output() const {
        return std::count_if(m_outputs.begin(), m_outputs.end(),
            [](const OPort &oport) { return !oport.empty(); });
    }

    inline bool inputs_full() const { return num_unconnected_input() == 0; }
    inline bool outputs_full() const { return num_unconnected_output() == 0; }

protected:

    std::vector<IPort> m_inputs;
    std::vector<OPort> m_outputs;

};

class IPin : public Atom {

public:

    IPin(std::size_t max_fanouts)
        :Atom{ 0, 1, max_fanouts }
    {}

    IPin(const IPin &other)
        :Atom{ other }
    {}

    IPin(IPin &&other)
        :Atom{ std::move(other) }
    {}

    IPin &operator=(const IPin &other) {
        Atom::operator=(other);
        return *this;
    }

    IPin &operator=(IPin &&other) {
        Atom::operator=(std::move(other));
        return *this;
    }

    inline OPort &get_oport() {
        return Atom::get_oport(0);
    }

    const inline OPort &get_oport() const {
        return Atom::get_oport(0);
    }

};

class OPin : public Atom {

public:

    OPin()
        :Atom{ 1, 0, 0 }
    {}

    OPin(const OPin &other)
        :Atom{ other }
    {}

    OPin(OPin &&other)
        :Atom{ std::move(other) }
    {}

    OPin &operator=(const OPin &other) {
        Atom::operator=(other);
        return *this;
    }

    OPin &operator=(OPin &&other) {
        Atom::operator=(std::move(other));
        return *this;
    }

    inline IPort &get_iport() {
        return Atom::get_iport(0);
    }

};


class Netlist {

    friend struct Utils::Access;

public:

    struct LUT {};
    struct FF {};

    Netlist(std::size_t num_ipins, std::size_t num_opins, std::size_t num_luts, std::size_t num_ffs,
            std::size_t max_inputs, std::size_t max_outputs, std::size_t max_fanouts)
        :m_luts( num_luts, Atom{ max_inputs, max_outputs, max_fanouts } ),
        m_ffs( num_ffs, Atom{ max_inputs, max_outputs, max_fanouts } ),
        m_ipins( num_ipins, IPin{ max_fanouts } ),
        m_opins( num_opins )
    {
        init_back_pointers();
    }

    Netlist(Netlist &&other)
        :m_luts{ std::move(other.m_luts) },
        m_ffs{ std::move(other.m_ffs) },
        m_ipins{ std::move(other.m_ipins) },
        m_opins{ std::move(other.m_opins) }
    {}

    inline Netlist &operator=(Netlist &&other) {
        m_luts = std::move(other.m_luts);
        m_ffs = std::move(other.m_ffs);
        m_ipins = std::move(other.m_ipins);
        m_opins = std::move(other.m_opins);
        return *this;
    }

    inline auto begin_luts() { return m_luts.begin(); }
    inline auto begin_luts() const { return m_luts.begin(); }
    inline auto end_luts() { return m_luts.end(); }
    inline auto end_luts() const { return m_luts.end(); }

    inline auto luts() { return boost::make_iterator_range(begin_luts(), end_luts()); }
    inline auto luts() const { return boost::make_iterator_range(begin_luts(), end_luts()); }

    inline auto begin_ffs() { return m_ffs.begin(); }
    inline auto begin_ffs() const { return m_ffs.begin(); }
    inline auto end_ffs() { return m_ffs.end(); }
    inline auto end_ffs() const { return m_ffs.end(); }

    inline auto ffs() { return boost::make_iterator_range(begin_ffs(), end_ffs()); }
    inline auto ffs() const { return boost::make_iterator_range(begin_ffs(), end_ffs()); }

    inline auto begin_ipins() { return m_ipins.begin(); }
    inline auto begin_ipins() const { return m_ipins.begin(); }
    inline auto end_ipins() { return m_ipins.end(); }
    inline auto end_ipins() const { return m_ipins.end(); }

    inline auto ipins() { return boost::make_iterator_range(begin_ipins(), end_ipins()); }
    inline auto ipins() const { return boost::make_iterator_range(begin_ipins(), end_ipins()); }

    inline auto begin_opins() { return m_opins.begin(); }
    inline auto begin_opins() const { return m_opins.begin(); }
    inline auto end_opins() { return m_opins.end(); }
    inline auto end_opins() const { return m_opins.end(); }

    inline auto opins() { return boost::make_iterator_range(begin_opins(), end_opins()); }
    inline auto opins() const { return boost::make_iterator_range(begin_opins(), end_opins()); }

    inline auto num_luts() const { return m_luts.size(); }
    inline auto num_ffs() const { return m_ffs.size(); }
    inline auto num_ipins() const { return m_ipins.size(); }
    inline auto num_opins() const { return m_opins.size(); }

private:

    void init_back_pointers() {
        auto init = [](Atom &atom) { atom.init_back_pointer(); };
        std::for_each(m_luts.begin(), m_luts.end(), init);
        std::for_each(m_ffs.begin(), m_ffs.end(), init);
        std::for_each(m_ipins.begin(), m_ipins.end(), init);
        std::for_each(m_opins.begin(), m_opins.end(), init);
    }

    std::vector<Atom> m_luts;
    std::vector<Atom> m_ffs;
    std::vector<IPin> m_ipins;
    std::vector<OPin> m_opins;

};

namespace Utils {

    struct Access {

        static auto &get_luts(Netlist &netlist) { return netlist.m_luts; }
        static const auto &get_luts(const Netlist &netlist) { return netlist.m_luts; }
        static auto &get_ffs(Netlist &netlist) { return netlist.m_ffs; }
        static const auto &get_ffs(const Netlist &netlist) { return netlist.m_ffs; }
        static auto &get_ipins(Netlist &netlist) { return netlist.m_ipins; }
        static const auto &get_ipins(const Netlist &netlist) { return netlist.m_ipins; }
        static auto &get_opins(Netlist &netlist) { return netlist.m_opins; }
        static const auto &get_opins(const Netlist &netlist) { return netlist.m_opins; }

    };

    static inline void connect(OPort &oport, IPort &iport) {
        RUNTIME_ASSERT(!iport.has_fanin());
        oport.push_back(iport);
        iport.fanin() = &oport;
    }

    static inline void connect(IPort &iport, OPort &oport) {
        connect(oport, iport);
    }

    static inline void connect(IPin &ipin, IPort &iport) {
        connect(ipin.get_oport(), iport);
    }

    static inline void connect(IPort &iport, IPin &ipin) {
        connect(ipin, iport);
    }

    static inline void connect(OPin &opin, OPort &oport) {
        connect(opin.get_iport(), oport);
    }

    static inline void connect(OPort &oport, OPin &opin) {
        connect(opin, oport);
    }

    template <typename T>
    auto &get(Netlist &netlist, std::size_t idx) {}
    template <typename T>
    const auto &get(const Netlist &netlist, std::size_t idx) {}

    template <>
    inline auto &get<Netlist::LUT>(Netlist &netlist, std::size_t idx) { return Access::get_luts(netlist)[idx]; }
    template <>
    inline const auto &get<Netlist::LUT>(const Netlist &netlist, std::size_t idx) { return Access::get_luts(netlist)[idx]; }

    template <>
    inline auto &get<Netlist::FF>(Netlist &netlist, std::size_t idx) { return Access::get_ffs(netlist)[idx]; }
    template <>
    inline const auto &get<Netlist::FF>(const Netlist &netlist, std::size_t idx) { return Access::get_ffs(netlist)[idx]; }

    template <>
    inline auto &get<IPin>(Netlist &netlist, std::size_t idx) { return Access::get_ipins(netlist)[idx]; }
    template <>
    inline const auto &get<IPin>(const Netlist &netlist, std::size_t idx) { return Access::get_ipins(netlist)[idx]; }

    template <>
    inline auto &get<OPin>(Netlist &netlist, std::size_t idx) { return Access::get_opins(netlist)[idx]; }
    template <>
    inline const auto &get<OPin>(const Netlist &netlist, std::size_t idx) { return Access::get_opins(netlist)[idx]; }

    Netlist random_netlist(std::size_t num_ipins, std::size_t num_opins, std::size_t num_luts,
        std::size_t num_ffs, std::size_t num_inputs, std::size_t num_outputs);

    void dump_netlist(const Netlist &netlist, const std::string &filepath);

};