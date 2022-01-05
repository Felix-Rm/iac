#pragma once

#include "std_provider/vector.hpp"
#ifndef ARDUINO

#include <filesystem>
#include <fstream>
#include <vector>

#include "../node.hpp"
#include "../std_provider/printf.hpp"
#include "../std_provider/string.hpp"
#include "../std_provider/unordered_map.hpp"
#include "../transport_routes/socket_transport_route.hpp"

#ifndef VISUALIZATION_SITE_DIRECTORY
#define VISUALIZATION_SITE_DIRECTORY "iac/network_visualization/site/"
#endif

namespace iac {

class Visualization {
   public:
    Visualization(const char* ip, int port, const char* site_path = VISUALIZATION_SITE_DIRECTORY);
    ~Visualization() = default;

    void add_node(string name, LocalNode& node);
    void remove_node(LocalNode& node);
    void remove_node(string name);

    void update();

   private:
    std::filesystem::path parseRequest();
    void answerRequest(const std::filesystem::path& path);

    unordered_map<string, LocalNode*> m_nodes;
    SocketServerTransportRoute m_server;

    const char* m_path;

    char m_message_buffer[2048]{0};
    char* m_buffer_pointer = m_message_buffer;

    void sendHeader(const char* status, size_t size, const char* mime_type);

    ::std::filesystem::path m_root_path = VISUALIZATION_SITE_DIRECTORY;

    unordered_map<string, string> m_mime_types;
};

}  // namespace iac

#endif