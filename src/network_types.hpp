#pragma once

#include <cstdint>

#include "logging.hpp"
#include "std_provider/limits.hpp"
#include "std_provider/printf.hpp"
#include "std_provider/utility.hpp"

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
    DISCONNECT = numeric_limits<package_type_t>::max() - 1,
    HEARTBEAT = numeric_limits<package_type_t>::max() - 2,
};

enum reserved_endpoint_addresses {
    IAC = numeric_limits<ep_id_t>::max(),
};

constexpr uint8_t unset_id = reserved_endpoint_addresses::IAC;

#ifndef IAC_DISABLE_EXCEPTIONS
IAC_CREATE_MESSAGE_EXCEPTION(EmptyNetworkEntryDereferenceException);
IAC_CREATE_MESSAGE_EXCEPTION(CopyingNonEmptyNetworkEntryException);
IAC_CREATE_MESSAGE_EXCEPTION(BindingToNonEmptyNetworkEntryException);
#endif

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
            move_from(move(other));
        return *this;
    }

    ManagedNetworkEntry& operator=(const ManagedNetworkEntry& other) {
        if (&other != this)
            copy_from(other);
        return *this;
    }

    void bind(T& element) {
        if (m_elt) {
            IAC_HANDLE_EXCEPTION(BindingToNonEmptyNetworkEntryException, "binded on already populated entry");
        }
        m_elt = &element;
    };

    T& element() const {
        if (!m_elt) {
            IAC_HANDLE_FATAL_EXCEPTION(EmptyNetworkEntryDereferenceException, "used operator-> on empty network entry");
        }
        return *element_ptr();
    };

    T* element_ptr() const {
        return m_elt;
    };

   private:
    void move_from(ManagedNetworkEntry&& other) {
        if (this == &other) return;

        delete_element();
        bind(*other.m_elt);
        other.m_elt = nullptr;
    }

    void copy_from(const ManagedNetworkEntry& other) {
        if (other.m_elt) {
            IAC_HANDLE_EXCEPTION(CopyingNonEmptyNetworkEntryException, "tried copy on non empty network entry");
        }
        delete_element();
    }

    void delete_element() {
        if (m_elt && !m_elt->local())
            delete m_elt;
        m_elt = nullptr;
    }

    T* m_elt{nullptr};
};

}  // namespace iac