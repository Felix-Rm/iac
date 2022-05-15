#pragma once

#ifndef IAC_DISABLE_VISUALIZATION

#    include <fstream>

#    include "../logging.hpp"
#    include "../network.hpp"
#    include "../std_provider/printf.hpp"
#    include "../std_provider/string.hpp"
#    include "../std_provider/unordered_map.hpp"
#    include "../std_provider/vector.hpp"
#    include "../transport_routes/socket_transport_route.hpp"
#    include "json_writer.hpp"

#    ifndef VISUALIZATION_SITE_DIRECTORY
#        define VISUALIZATION_SITE_DIRECTORY "iac/network_visualization/site/"
#    endif

namespace iac {

class Visualization {
   public:
    Visualization(const char* ip, int port, const char* site_path = VISUALIZATION_SITE_DIRECTORY);

    void add_network(const string& name, const Network& network);
    void remove_network(const Network& network);
    void remove_network(const string& name);

    void update();

   private:
    // std::filesystem::path created problems with linters etc.
    // as no special features of std::filesystem are needed, just use string for now
    typedef string path;

    path parse_request();
    void answer_request(const path& path);

    string get_mime_type(const path& file_path);

    unordered_map<string, const Network*> m_networks;
    SocketServerTransportRoute m_server;

    size_t m_failed_reads = 0;
    const char* m_path;

    static constexpr unsigned s_message_buffer_size = 2048;
    char m_message_buffer[s_message_buffer_size]{0};
    size_t m_buffer_index{0};

    void send_header(const char* status, size_t size, const char* mime_type);

    path m_root_path = VISUALIZATION_SITE_DIRECTORY;

    unordered_map<string, string> m_mime_types;
};

}  // namespace iac

#endif