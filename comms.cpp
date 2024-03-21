#include "monitor.h"
#include "comms.h"

#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::udp;

#include <nlohmann/json.hpp>

// shortening the namespace for convenience
using namespace boost::asio::ip;
using json = nlohmann::json;

void send_update (const std::string& service_ip, const int op, const std::string& arch) {
    json msg = {};
    msg["address"] = service_ip;
    msg["status"] = op;
    msg["provider_architecture"] = arch;
    msg["timestamp"] = get_timestamp();
    std::string message = msg.dump();

    std::cout << message << std::endl;

    try {
        boost::asio::io_service io_service;
        udp::socket socket(io_service);
        udp::endpoint local_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 10000);

        socket.open(udp::v4());
        socket.send_to(boost::asio::buffer(message), local_endpoint);

        // Clean up
        socket.close();
    } catch (const std::exception& e) {
        // Handle exception
        std::cerr << "Error: " << e.what() << "\n";
    }
}
