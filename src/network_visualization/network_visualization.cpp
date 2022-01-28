
#include <unordered_map>

#include "network_types.hpp"
#include "std_provider/string.hpp"

#ifndef ARDUINO

#    include "network_visualization/network_visualization.hpp"

namespace iac {

Visualization::Visualization(const char* ip, int port, const char* site_path) : m_server(ip, port), m_path(site_path) {
    if (!m_server) {
        iac_log(Logging::loglevels::debug, "error creating webserver\n");
    }

    m_mime_types[".html"] = "text/html; charset=utf-8";
    m_mime_types[".ico"] = "image/x-icon";
    m_mime_types[".js"] = "application/javascript";
    m_mime_types[".css"] = "text/css";
    m_mime_types[".txt"] = "text/plain";
}

void Visualization::add_node(const string& name, LocalNode& node) {
    m_nodes[name] = &node;
}

void Visualization::remove_node(const LocalNode& node) {
    for (auto it = m_nodes.begin(); it != m_nodes.end();) {
        if (it->second == &node)
            it = m_nodes.erase(it);
        else
            it++;
    }
}

void Visualization::remove_node(const string& name) {
    m_nodes.erase(name);
}

void Visualization::update() {
    if (m_server.m_state != LocalTransportRoute::route_state::OPEN) {
        m_server.m_state = m_server.open() ? LocalTransportRoute::route_state::OPEN : LocalTransportRoute::route_state::CLOSED;
    }

    if (m_server.m_state == LocalTransportRoute::route_state::OPEN) {
        path result;

        while (!(result = parse_request()).empty())
            answer_request(result);
    }
}

Visualization::path Visualization::parse_request() {
    static constexpr unsigned path_start_index = 5;

    size_t available = m_server.available();
    path requested_path;

    if (available > 0) {
        m_server.read(&m_message_buffer[m_buffer_index], available);
        m_buffer_index += available;
        // printf("request in\n");

        // check if entire request has been read (http requests end with two newlines)
        if (m_buffer_index > path_start_index && strncmp("\r\n\r\n", &m_message_buffer[m_buffer_index - path_start_index], path_start_index - 1) == 0) {
            // reset buffer_pointer
            m_buffer_index = 0;

            // check for type of request
            if (m_message_buffer[0] == 'G') {
                for (size_t i = path_start_index; i < s_message_buffer_size && m_message_buffer[i] != ' ' && m_message_buffer[i] != '?'; i++)
                    requested_path += m_message_buffer[i];

                if (requested_path.empty())
                    return "index.html";

                return requested_path;
            }
        }
    }

    return {};
}
void Visualization::answer_request(const Visualization::path& path) {
    auto full_path = m_root_path + path;

    // printf("path: %s %s\n", full_path.c_str(), full_path.extension().c_str());

    if (path == "data") {
        string data;

        for (const auto& map_entry : m_nodes) {
            auto& node = *map_entry.second;

            int node_id_counter = 0;
            std::unordered_map<iac::node_id_t, int> node_id_mapping;

            data += ":$" + map_entry.first + "\n";

            for (auto& entry : node.m_node_mapping) {
                node_id_mapping[entry.first] = node_id_counter++;
                data += "#$" + std::to_string(node_id_mapping[entry.first]);

                for (const auto& endpoint_id : entry.second->endpoints()) {
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

        send_header("200 OK", data.length(), m_mime_types[".txt"].c_str());
        m_server.write(data.c_str(), data.length());

    } else {
        std::ifstream input(full_path, std::ios::binary);

        if (!input) {
            send_header("404 NOT FOUND", 0, "");

        } else {
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

            send_header("200 OK", buffer.size(), get_mime_type(full_path).c_str());
            m_server.write(buffer.data(), buffer.size());
        }
    }

    m_server.flush();
    m_server.m_state = m_server.close() ? LocalTransportRoute::route_state::CLOSED : LocalTransportRoute::route_state::OPEN;
    // printf("closed %d\n", m_server.state == TransportRoute::route_state::CLOSED);
}

string Visualization::get_mime_type(const Visualization::path& file_path) {
    for (size_t i = file_path.length(); i >= 0; --i) {
        if (file_path[i] == '.')
            return m_mime_types[file_path.substr(i)];
        if (file_path[i] == '/' || file_path[i] == '\\')
            break;
    }

    return "text/plain";
}

void Visualization::send_header(const char* status, size_t size, const char* mime_type) {
    static constexpr unsigned buff_size = 1024;

    char buffer[buff_size];
    size_t written = snprintf(buffer, buff_size, "HTTP/1.1 %s\r\nContent-Length: %lu\r\nContent-Type: %s\r\n\r\n", status, size, mime_type);

    m_server.write(buffer, written);
}
}  // namespace iac
#endif