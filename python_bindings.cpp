#ifdef COMPILE_PYTHON_BINDINGS

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>

#include "network_visualization/network_visualization.hpp"
#include "node.hpp"

#define method(c, x) def(#x, &c::x)

PYBIND11_MODULE(iac_py, module_handle) {
    module_handle.doc() = "inter<anything>comunication python_wrapper";

    pybind11::class_<iac::LocalNode>(module_handle, "Node")
        .def(pybind11::init<const char*, uint16_t, uint16_t>())
        .method(iac::LocalNode, print_endpoint_list)
        .method(iac::LocalNode, is_endpoint_connected)
        .method(iac::LocalNode, update);

    pybind11::enum_<iac::Package::buffer_management_t>(module_handle, "buffer_management_t");

    pybind11::class_<iac::LocalEndpoint>(module_handle, "LocalEndpoint")
        .def(pybind11::init<iac::LocalNode&, iac::Package::address_t, const char*>())

        .method(iac::LocalEndpoint, id)
        .method(iac::LocalEndpoint, name)
        .method(iac::LocalEndpoint, remove_package_handler)
        // .def("send", pybind11::overload_cast<iac::Package::address_t, iac::Package::package_type_t, const iac::BufferWriter&, iac::Package::buffer_management_t>(&iac::LocalEndpoint::send),
        //      pybind11::arg("to"), pybind11::arg("type"), pybind11::arg("buffer"),
        //      pybind11::arg("buffer_management") = iac::Package::buffer_management::IN_PLACE)
        // .def("send", pybind11::overload_cast<iac::Package::address_t, iac::Package::package_type_t, const uint8_t*, size_t, iac::Package::buffer_management_t>(&iac::LocalEndpoint::send),
        //      pybind11::arg("to"), pybind11::arg("type"), pybind11::arg("buffer"), pybind11::arg("buffer_length"),
        //      pybind11::arg("buffer_management") = iac::Package::buffer_management::IN_PLACE)
        .def("add_package_handler_by_buffer", pybind11::overload_cast<iac::Package::address_t, std::function<void(const iac::Package& pkg)>>(&iac::LocalEndpoint::add_package_handler))
        .def("add_package_handler_by_reader", pybind11::overload_cast<iac::Package::address_t, std::function<void(const iac::Package& pkg, iac::BufferReader&&)>>(&iac::LocalEndpoint::add_package_handler));

    pybind11::class_<iac::Package>(module_handle, "Package")
        .def(pybind11::init<iac::Package::address_t, iac::Package::address_t, iac::Package::package_type_t, const uint8_t*, size_t, iac::Package::buffer_management_t>())

        .def("from_", &iac::Package::from)
        .method(iac::Package, to)
        .method(iac::Package, type)
        .method(iac::Package, payload)
        .method(iac::Package, payload_size)
        .method(iac::Package, route);

    pybind11::class_<iac::BufferReader>(module_handle, "BufferReader")
        .def(pybind11::init<const uint8_t*, size_t>())

        .method(iac::BufferReader, buffer)
        .method(iac::BufferReader, size)
        .method(iac::BufferReader, str)
        .method(iac::BufferReader, boolean)
        .def("num_uint8_t", &iac::BufferReader::num<uint8_t>)
        .def("num_uint16_t", &iac::BufferReader::num<uint16_t>)
        .def("num_int8_t", &iac::BufferReader::num<int8_t>)
        .def("num_int16_t", &iac::BufferReader::num<int16_t>);

    pybind11::class_<iac::BufferWriter>(module_handle, "BufferWriter")
        .def(pybind11::init<>())
        .def(pybind11::init<uint8_t*, size_t>())

        .method(iac::BufferWriter, buffer)
        .method(iac::BufferWriter, size)
        .def("str", pybind11::overload_cast<const char*>(&iac::BufferWriter::str))
        .def("str", pybind11::overload_cast<iac::string>(&iac::BufferWriter::str))
        .method(iac::BufferWriter, boolean)
        .def("num_uint8_t", &iac::BufferWriter::num<uint8_t>)
        .def("num_uint16_t", &iac::BufferWriter::num<uint16_t>)
        .def("num_int8_t", &iac::BufferWriter::num<int8_t>)
        .def("num_int16_t", &iac::BufferWriter::num<int16_t>);

    pybind11::class_<iac::Visualization>(module_handle, "Visualization")
        .def(pybind11::init<const iac::LocalNode&, const char*, int, const char*>())
        .def(pybind11::init<const iac::LocalNode&, const char*, int>())

        .method(iac::Visualization, update);

    pybind11::class_<iac::LocalTransportRoute>(module_handle, "TransportRoute");

    pybind11::class_<iac::SocketClientTransportRoute, iac::LocalTransportRoute>(module_handle, "SocketClientTransportRoute")
        .def(pybind11::init<const char*, int>());

    pybind11::class_<iac::SocketServerTransportRoute, iac::LocalTransportRoute>(module_handle, "SocketServerTransportRoute")
        .def(pybind11::init<const char*, int>());
}

#endif