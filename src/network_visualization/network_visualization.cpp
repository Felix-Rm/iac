#ifndef ARDUINO

#    include "network_visualization/network_visualization.hpp"

namespace iac {

Visualization::Visualization(const char* ip, int port, const char* site_path)
    : m_server(ip, port), m_path(site_path) {
    if (!m_server) {
        iac_log(Logging::loglevels::debug, "error creating webserver\n");
    }

    m_mime_types[".html"] = "text/html; charset=utf-8";
    m_mime_types[".ico"] = "image/x-icon";
    m_mime_types[".js"] = "application/javascript";
    m_mime_types[".css"] = "text/css";
    m_mime_types[".txt"] = "text/plain";
    m_mime_types[".json"] = "application/json";
}

void Visualization::add_network(const string& name, const Network& network) {
    m_networks[name] = &network;
}

void Visualization::remove_network(const Network& network) {
    for (auto it = m_networks.begin(); it != m_networks.end();) {
        if (it->second == &network)
            it = m_networks.erase(it);
        else
            it++;
    }
}

void Visualization::remove_network(const string& name) {
    m_networks.erase(name);
}

void Visualization::update() {
    if (m_server.state() != LocalTransportRoute::route_state::OPEN) {
        m_server.state() = m_server.open() ? LocalTransportRoute::route_state::OPEN : LocalTransportRoute::route_state::CLOSED;
        m_failed_reads = 0;
    }

    if (m_server.state() == LocalTransportRoute::route_state::OPEN) {
        path result = parse_request();

        if (!result.empty())
            answer_request(result);
    }
}

Visualization::path Visualization::parse_request() {
    static constexpr unsigned path_start_index = 5;
    static constexpr unsigned request_end_signal_length = 4;

    size_t available = m_server.available();
    path requested_path;

    if (available > 0) {
        m_server.read(&m_message_buffer[m_buffer_index], available);
        m_buffer_index += available;
        // printf("request in\n");

        // check if entire request has been read (http requests end with two newlines)
        if (m_buffer_index > path_start_index && strncmp("\r\n\r\n", &(m_message_buffer[m_buffer_index - request_end_signal_length]), request_end_signal_length) == 0) {
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
        JSONObject data{};

        for (const auto& map_entry : m_networks) {
            JSONObject network_data{};
            const auto& network = *map_entry.second;

            int node_id_counter = 0;
            unordered_map<iac::node_id_t, int> node_id_mapping;

            JSONArray network_node_list{};

            for (const auto& entry : network.node_mapping()) {
                auto node_id = node_id_counter++;
                node_id_mapping[entry.first] = node_id;

                JSONObject node_data{};

                node_data.add("id", JSONValue{entry.second->id()});

                JSONArray node_endpoint_data{};

                for (const auto& endpoint_id : entry.second->endpoints()) {
                    const auto& ep = network.endpoint(endpoint_id);
                    JSONObject endpoint_data{};

                    endpoint_data.add("id", JSONValue{ep.id()});
                    endpoint_data.add("name", JSONValue{ep.name()});

                    node_endpoint_data.add(JSONValue{iac::move(endpoint_data)});
                }

                node_data.add("endpoints", JSONValue{iac::move(node_endpoint_data)});
                network_node_list.add(JSONValue{iac::move(node_data)});
            }

            network_data.add("nodes", JSONValue{iac::move(network_node_list)});

            JSONArray network_route_list{};

            for (const auto& entry : network.route_mapping()) {
                const auto& tr = entry.second.element();
                JSONObject route_data{};

                route_data.add("id", JSONValue{tr.id()});
                route_data.add("source", JSONValue{node_id_mapping[tr.node1()]});
                route_data.add("target", JSONValue{node_id_mapping[tr.node2()]});
                route_data.add("node1", JSONValue{tr.node1()});
                route_data.add("node2", JSONValue{tr.node2()});
                route_data.add("typestring", JSONValue{tr.typestring()});
                route_data.add("infostring", JSONValue{tr.infostring()});

                network_route_list.add(JSONValue{iac::move(route_data)});
            }

            network_data.add("routes", JSONValue{iac::move(network_route_list)});

            data.add(map_entry.first, JSONValue{iac::move(network_data)});
        }

        string json = data.stringify();

        send_header("200 OK", json.length(), m_mime_types[".json"].c_str());
        m_server.write(json.c_str(), json.length());

    } else {
        std::ifstream input(full_path, std::ios::binary);

        if (!input) {
            send_header("404 NOT FOUND", 0, "");

        } else {
            vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

            send_header("200 OK", buffer.size(), get_mime_type(full_path).c_str());
            m_server.write(buffer.data(), buffer.size());
        }
    }

    m_server.flush();
    m_server.state() = m_server.close() ? LocalTransportRoute::route_state::CLOSED : LocalTransportRoute::route_state::OPEN;
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