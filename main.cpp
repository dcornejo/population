#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

#include <nlohmann/json.hpp>

#include "monitor.h"
#include "utilities.h"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

/**
 * @brief Transmits a message to a multicast group.
 *
 * This function creates a multicast socket and continuously sends the provided message to the specified
 * multicast group. The message is sent every 500 milliseconds until the program is terminated.
 *
 * @param group_ip The IP address of the multicast group to send the message to.
 * @param group_port The network port of the multicast group.
 */
void transmit_thread (const char *group_ip, unsigned short group_port) {
    int sock = new_multicast_socket(group_ip);

    ordered_json j;

    auto sys_info = new system_info;
    auto address = get_interface_address();

    // basic identification
    j["id"] = sys_info->nodename;
    j["address"] = address;
    // j["first_seen"] = 0;
    // j["last_seen"] = 0;

    // role information
    j["active"] = true;

    j["provides"] = {"msmtpd", "video"};

    // participant information
    j["operating_system"] = sys_info->sysname;
    j["release"] = sys_info->release;
    j["architecture"] = sys_info->machine;

    std::string message = j.dump(4);

    struct sockaddr_in group_addr = {};
    memset((char *)&group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr(group_ip);
    group_addr.sin_port = htons(group_port);

    while (true) {
        if (sendto(sock, message.c_str(), message.size(), 0, reinterpret_cast<struct sockaddr *>(&group_addr),
                   sizeof (group_addr)) < 0) {
            perror("Sending datagram message error");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/**
 * @brief Receives and processes multicast datagrams.
 *
 * This function creates a multicast socket, binds it to the specified group IP and port,
 * and then continuously receives and processes multicast datagrams. The received messages
 * are stored in a map along with their source and timestamp.
 *
 * @param group_ip The IP address of the multicast group to join.
 * @param group_port The port number of the multicast group to join.
 * @throws std::runtime_error If any error occurs while creating or binding the socket.
 */
void receive_thread (const char *group_ip, unsigned short group_port) {
    int sock = new_multicast_socket(group_ip);

    struct sockaddr_in group_addr = {};
    memset(&group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    group_addr.sin_port = htons(group_port);

    if (bind(sock, reinterpret_cast<struct sockaddr *>(&group_addr), sizeof (group_addr)) < 0) {
        throw std::runtime_error("Binding datagram socket error");
    }

    char buffer[1024];
    struct sockaddr_in src_addr = {};

    memset(&src_addr, 0, sizeof (src_addr));

    socklen_t src_addr_len = sizeof (src_addr);

    while (true) {
        if (ssize_t received = recvfrom(sock, buffer, sizeof (buffer), 0,
                                        reinterpret_cast<struct sockaddr *>(&src_addr), &src_addr_len); received < 0) {
            perror("Receiving datagram message error");
            break;
        }
        else {
            buffer[received] = '\0';
            auto x = nlohmann::ordered_json::parse(buffer);
            report_participant(x);
        }
    }
}

/**
 * @brief Expire thread to remove expired messages from messages_map.
 *
 * This function runs in an infinite loop, continuously checking the timestamp of each message in messages_map.
 * If a message is older than 5 seconds, it is removed from the map.
 *
 * @note The messages_map is defined as a std::map<std::string, std::pair<std::string, time_t>>.
 *
 * @param None.
 * @return None.
 */

constexpr std::int64_t EXPIRY_MS = 1000;

void expire_thread () {
    while (true) {
        // auto now = get_timestamp();
        check_participants();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

/**
 * @file main.cpp
 * @brief This file contains the main function which creates and joins threads for transmitting, receiving, and expiration.
 */
int main () {
    std::thread multicastSender(transmit_thread, "224.1.1.1", 50000);
    std::thread multicastReceiver(receive_thread, "224.1.1.1", 50000);
    std::thread expiryScanner(expire_thread);

    multicastSender.join();
    multicastReceiver.join();
    expiryScanner.join();

    return 0;
}
