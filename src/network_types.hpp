#pragma once

#include "exceptions.hpp"
#include "forward.hpp"
#include "logging.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/string.hpp"
#include "std_provider/unordered_map.hpp"
#include "std_provider/unordered_set.hpp"
#include "std_provider/utility.hpp"

#ifndef ARDUINO
#    include <chrono>
#endif

namespace iac {

typedef uint8_t node_id_t;
typedef uint16_t tr_id_t;
typedef uint8_t ep_id_t;

typedef uint8_t package_type_t;
typedef uint16_t package_size_t;
typedef uint8_t metadata_t;
typedef uint8_t start_byte_t;

enum reserved_package_types {
    CONNECT = numeric_limits<package_type_t>::max(),
    ACK = numeric_limits<package_type_t>::max() - 1,
    NETWORK_UPDATE = numeric_limits<package_type_t>::max() - 2,
    HEARTBEAT = numeric_limits<package_type_t>::max() - 3,
};

enum reserved_endpoint_addresses {
    IAC = numeric_limits<ep_id_t>::max(),
};

constexpr uint8_t unset_id = reserved_endpoint_addresses::IAC;

typedef struct route_timings {
    uint16_t heartbeat_interval_ms = 0;
    uint16_t assume_dead_after_ms = 0;
} route_timings_t;

struct timestamp {
    timestamp() = default;

    timestamp(size_t ts)
        : ts(ts){};

    static timestamp now() {
#ifdef ARDUINO
        return millis();
#else
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
    }

    bool is_more_than_n_in_past(const timestamp& now, const size_t n) {
        return timestamp(ts + n) < now;
    }

    bool operator<(const timestamp& rhs) {
        return ts < rhs.ts;
    }

    size_t operator-(const timestamp& rhs) {
        return ts - rhs.ts;
    }

    size_t ts;
};

IAC_MAKE_EXCEPTION(EmptyNetworkEntryDereferenceException);
IAC_MAKE_EXCEPTION(CopyingNonEmptyNetworkEntryException);
IAC_MAKE_EXCEPTION(BindingToNonEmptyNetworkEntryException);

IAC_MAKE_EXCEPTION(RemoveOfInvalidException);
IAC_MAKE_EXCEPTION(NoRegisteredEndpointsException);
IAC_MAKE_EXCEPTION(AddDuplicateException);
IAC_MAKE_EXCEPTION(NonExistingException);
IAC_MAKE_EXCEPTION(NetworkStateException);

template <typename T>
class ManagedNetworkEntry {
   public:
    ManagedNetworkEntry() = default;
    ManagedNetworkEntry(const ManagedNetworkEntry& other) {
        copy_from(other);
    };

    ManagedNetworkEntry(ManagedNetworkEntry&& other) {
        move_from(other);
    };

    ~ManagedNetworkEntry() {
        delete_element();
    };

    T* operator->() {
        if (!m_elt) {
            IAC_HANDLE_FATAL_EXCEPTION(EmptyNetworkEntryDereferenceException, "used operator-> on empty network entry");
        }

        return m_elt;
    }

    const T* operator->() const {
        if (!m_elt) {
            IAC_HANDLE_FATAL_EXCEPTION(EmptyNetworkEntryDereferenceException, "used operator-> on empty network entry");
        }

        return m_elt;
    }

    ManagedNetworkEntry& operator=(ManagedNetworkEntry&& other) {
        if (&other != this)
            move_from(other);
        return *this;
    }

    ManagedNetworkEntry& operator=(const ManagedNetworkEntry& other) {
        if (&other != this)
            copy_from(other);
        return *this;
    }

    void bind(T& element) {
        if (m_elt) {
            IAC_HANDLE_EXCEPTION(BindingToNonEmptyNetworkEntryException, "bind on already populated entry");
        }
        m_is_adopted = false;
        m_elt = &element;
    };

    void adopt(T& element) {
        if (m_elt) {
            IAC_HANDLE_EXCEPTION(BindingToNonEmptyNetworkEntryException, "adopt on already populated entry");
        }
        m_is_adopted = true;
        m_elt = &element;
    };

    inline void bind(T* element) {
        bind(*element);
    };

    inline void adopt(T* element) {
        adopt(*element);
    };

    static ManagedNetworkEntry<T> create_and_bind(T& element) {
        ManagedNetworkEntry<T> entry;
        entry.bind(element);
        return entry;
    }

    static ManagedNetworkEntry<T> create_and_bind(T* element) {
        ManagedNetworkEntry<T> entry;
        entry.bind(element);
        return entry;
    }

    static ManagedNetworkEntry<T> create_and_adopt(T& element) {
        ManagedNetworkEntry<T> entry;
        entry.adopt(element);
        return entry;
    }

    static ManagedNetworkEntry<T> create_and_adopt(T* element) {
        ManagedNetworkEntry<T> entry;
        entry.adopt(element);
        return entry;
    }

    T& element() {
        if (!m_elt) {
            IAC_HANDLE_FATAL_EXCEPTION(EmptyNetworkEntryDereferenceException, "used operator-> on empty network entry");
        }
        return *element_ptr();
    };

    T* element_ptr() {
        return m_elt;
    };

    const T& element() const {
        if (!m_elt) {
            IAC_HANDLE_FATAL_EXCEPTION(EmptyNetworkEntryDereferenceException, "used operator-> on empty network entry");
        }
        return *element_ptr();
    };

    const T* element_ptr() const {
        return m_elt;
    };

   private:
    void move_from(ManagedNetworkEntry& other) {
        delete_element();
        m_elt = other.m_elt;
        m_is_adopted = other.m_is_adopted;
        other.m_elt = nullptr;
    }

    void copy_from(const ManagedNetworkEntry& other) {
        if (other.m_elt) {
            IAC_HANDLE_EXCEPTION(CopyingNonEmptyNetworkEntryException, "tried copy on non empty network entry");
        }
        delete_element();
    }

    void delete_element() {
        if (m_elt && m_is_adopted)
            delete m_elt;
        m_elt = nullptr;
    }

    T* m_elt{nullptr};
    bool m_is_adopted = false;
};

class Node {
    friend Network;
    friend LocalNode;

   public:
    virtual ~Node() = default;

    typedef unordered_set<ep_id_t> endpoint_list_t;
    typedef unordered_set<tr_id_t> route_list_t;
    typedef unordered_map<tr_id_t, uint8_t> local_route_list_t;

    [[nodiscard]] node_id_t id() const {
        return m_id;
    };

    [[nodiscard]] bool local() const {
        return m_local;
    };

    [[nodiscard]] const auto& endpoints() const {
        return m_endpoints;
    }

    [[nodiscard]] const auto& routes() const {
        return m_routes;
    }

    [[nodiscard]] const auto& local_routes() const {
        return m_local_routes;
    }

   protected:
    explicit Node(node_id_t id)
        : m_id(id){};

    void set_id(ep_id_t tr_id) {
        m_id = tr_id;
    };

    void set_local(bool local) {
        m_local = local;
    };

    void add_endpoint(ep_id_t ep_id) {
        m_endpoints.insert(ep_id);
    }

    void add_route(tr_id_t tr_id) {
        m_routes.insert(tr_id);
    }

    void add_local_route(tr_id_t tr_id, uint8_t hops) {
        m_local_routes.insert({tr_id, hops});
    }

    void add_local_route(pair<tr_id_t, uint8_t> entry) {
        m_local_routes.insert(entry);
    }

    void remove_endpoint(ep_id_t ep_id) {
        m_endpoints.erase(ep_id);
    }

    void remove_route(tr_id_t tr_id) {
        m_routes.erase(tr_id);
    }

    void remove_local_route(tr_id_t tr_id) {
        m_local_routes.erase(tr_id);
    }

    void remove_endpoint(const endpoint_list_t::const_iterator& ep_it) {
        m_endpoints.erase(ep_it);
    }

    void remove_route(const route_list_t::const_iterator& tr_it) {
        m_routes.erase(tr_it);
    }

    void remove_local_route(const local_route_list_t::const_iterator& tr_it) {
        m_local_routes.erase(tr_it);
    }

   private:
    node_id_t m_id{unset_id};
    bool m_local{false};

    endpoint_list_t m_endpoints{};
    route_list_t m_routes{};
    local_route_list_t m_local_routes{};
};

class Endpoint {
    friend Network;
    friend LocalNode;

   public:
    [[nodiscard]] ep_id_t id() const {
        return m_id;
    };

    [[nodiscard]] const string& name() const {
        return m_name;
    };

    [[nodiscard]] node_id_t node() const {
        return m_node;
    };

    [[nodiscard]] bool local() const {
        return m_local;
    };

   protected:
    explicit Endpoint(ep_id_t id, string name, node_id_t node = unset_id)
        : m_id(id), m_name(iac::move(name)), m_node(node){};

    explicit Endpoint(ep_id_t id, string&& name = "", node_id_t node = unset_id)
        : m_id(id), m_name(iac::move(name)), m_node(node){};

    void set_id(ep_id_t tr_id) {
        m_id = tr_id;
    };

    void set_local(bool local) {
        m_local = local;
    };

    void set_name(string& name) {
        m_name = name;
    };

    void set_name(string&& name) {
        m_name = iac::move(name);
    };

    void set_node(node_id_t node_id) {
        m_node = node_id;
    };

   private:
    ep_id_t m_id{unset_id};
    bool m_local{false};

    string m_name;
    node_id_t m_node{unset_id};
};

class TransportRoute {
    friend Network;
    friend LocalNode;

   public:
    [[nodiscard]] const string& typestring() const {
        return m_typestring;
    };

    [[nodiscard]] const string& infostring() const {
        return m_infostring;
    };

    [[nodiscard]] tr_id_t id() const {
        return m_id;
    };

    [[nodiscard]] bool local() const {
        return m_local;
    };

    [[nodiscard]] const auto& nodes() const {
        return m_nodes;
    };

    [[nodiscard]] const auto& node1() const {
        return m_nodes.first;
    };

    [[nodiscard]] const auto& node2() const {
        return m_nodes.second;
    };

    void printInfo() const {
        iac_log(Logging::loglevels::network, "[%s]", infostring().c_str());
    };

    void printTypestring() const {
        iac_log(Logging::loglevels::network, "'%s'", typestring().c_str());
    }

   protected:
    TransportRoute() = default;

    TransportRoute(id_t id, pair<node_id_t, node_id_t>&& nodes)
        : m_id(id), m_nodes(iac::move(nodes)){};

    void set_id(tr_id_t tr_id) {
        m_id = tr_id;
    };

    void set_local(bool local) {
        m_local = local;
    };

    void set_node1(node_id_t node_id) {
        m_nodes.first = node_id;
    };

    void set_node2(node_id_t node_id) {
        m_nodes.second = node_id;
    };

   private:
    tr_id_t m_id = unset_id;
    bool m_local = false;

    string m_typestring, m_infostring;
    pair<node_id_t, node_id_t> m_nodes{unset_id, unset_id};
};

}  // namespace iac