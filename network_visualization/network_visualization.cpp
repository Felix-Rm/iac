
#include <unordered_map>

#include "network_types.hpp"
#ifndef ARDUINO

#include "network_visualization.hpp"

namespace iac {

Visualization::Visualization(const char* ip, int port, const char* site_path) : m_server(ip, port), m_path(site_path) {
    if (!m_server) {
        iac_log(debug, "error creating webserver\n");
    }

    m_mime_types[".html"] = "text/html; charset=utf-8";
    m_mime_types[".ico"] = "image/x-icon";
    m_mime_types[".js"] = "application/javascript";
    m_mime_types[".css"] = "text/css";
    m_mime_types[".txt"] = "text/plain";
}

void Visualization::add_node(string name, LocalNode& node) {
    m_nodes[name] = &node;
}

void Visualization::remove_node(LocalNode& node) {
    for (auto it = m_nodes.begin(); it != m_nodes.end();) {
        if (it->second == &node)
            it = m_nodes.erase(it);
        else
            it++;
    }
}

void Visualization::remove_node(string name) {
    m_nodes.erase(name);
}

void Visualization::update() {
    if (m_server.m_state != LocalTransportRoute::route_state::OPEN) {
        m_server.m_state = m_server.open() ? LocalTransportRoute::route_state::OPEN : LocalTransportRoute::route_state::CLOSED;
    }

    if (m_server.m_state == LocalTransportRoute::route_state::OPEN) {
        std::filesystem::path result;

        while (!(result = parseRequest()).empty())
            answerRequest(result);
    }
}

std::filesystem::path Visualization::parseRequest() {
    size_t available = m_server.available();
    std::filesystem::path path;

    if (available) {
        m_server.read(m_buffer_pointer, available);
        m_buffer_pointer += available;
        // printf("request in\n");

        // ceck if entire request has been read (http requests end with two newlines)
        if (m_buffer_pointer - m_message_buffer > 5 && strncmp("\r\n\r\n", m_buffer_pointer - 5, 4)) {
            // reset buffer_pointer
            m_buffer_pointer = m_message_buffer;

            // check for type of request
            if (m_message_buffer[0] == 'G') {
                for (size_t i = 5; i < 2048 && m_message_buffer[i] != ' ' && m_message_buffer[i] != '?'; i++)
                    path += m_message_buffer[i];

                if (path.empty())
                    return "index.html";
                else
                    return path;
            } else {
                // printf("UNKNOWN\n");
            }

            // if (path.empty()) m_server.m_state = m_server.close() ? LocalTransportRoute::route_state::CLOSED : LocalTransportRoute::route_state::OPEN;
        }
    }

    return {};
}
void Visualization::answerRequest(const std::filesystem::path& path) {
    auto full_path = m_root_path / path;

    // printf("path: %s %s\n", full_path.c_str(), full_path.extension().c_str());

    if (path == "data") {
        string data;

        for (auto map_entry : m_nodes) {
            auto& node = *map_entry.second;

            int node_id_counter = 0;
            std::unordered_map<iac::node_id_t, int> node_id_mapping;

            data += ":$" + map_entry.first + "\n";

            for (auto& entry : node.m_node_mapping) {
                node_id_mapping[entry.first] = node_id_counter++;
                data += "#$" + std::to_string(node_id_mapping[entry.first]);

                for (auto& endpoint_id : entry.second->endpoints()) {
                    auto& ep = node.m_ep_mapping[endpoint_id].element();
                    data += '$' + std::to_string(ep.id()) + '$' + ep.name();
                }
                data += '\n';
            }

            for (auto& entry : node.m_tr_mapping) {
                data += "~$" + std::to_string(node_id_mapping[entry.second->node1()]) + '$' + std::to_string(node_id_mapping[entry.second->node2()]);
                data += '$' + entry.second.element().typestring() + '$' + entry.second.element().infostring() + '\n';
            }
        }

        sendHeader("200 OK", data.length(), m_mime_types[".txt"].c_str());
        m_server.write(data.c_str(), data.length());

    } else if (std::filesystem::is_regular_file(full_path)) {
        std::ifstream input(full_path, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

        sendHeader("200 OK", buffer.size(), m_mime_types[full_path.extension()].c_str());

        m_server.write(buffer.data(), buffer.size());
    } else {
        sendHeader("404 NOT FOUND", 0, "");
    }

    m_server.flush();
    m_server.m_state = m_server.close() ? LocalTransportRoute::route_state::CLOSED : LocalTransportRoute::route_state::OPEN;
    // printf("closed %d\n", m_server.state == TransportRoute::route_state::CLOSED);
}

void Visualization::sendHeader(const char* status, size_t size, const char* mime_type) {
    char buffer[1024];
    size_t written = snprintf(buffer, 1024, "HTTP/1.1 %s\r\nContent-Length: %lu\r\nContent-Type: %s\r\n\r\n", status, size, mime_type);

    m_server.write(buffer, written);
}
}  // namespace iac
#endif