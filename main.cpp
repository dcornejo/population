#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <mutex>
#include <chrono>
#include <ifaddrs.h>
#include <cerrno>

#include <nlohmann/json.hpp>

#include "monitor.h"

using json = nlohmann::json;

/*
 * map to store active peers
 */
std::map<std::string, std::pair<std::string, time_t>> messages_map;
std::mutex messages_mutex;

/**
 * @brief Prints the stored messages.
 *
 * This function prints the messages stored in the `messages_map`. It acquires a lock on `messages_mutex`
 * before accessing the map to ensure thread-safety.
 *
 * The format of the printed message is as follows:
 * Source: <source>
 * Message: <message>
 * Time: <time>
 *
 * Note: The messages are printed in no specific order.
 */
void print_messages () {
    std::lock_guard<std::mutex> lock (messages_mutex);

    for (auto &it: messages_map) {
        std::cout << "Source: " << it.first << "\n"
                  << "Message: " << it.second.first << "\n"
                  << "Time: " << it.second.second << "\n\n";
    }
}

/**
 * @brief Get the host name of the system.
 *
 * This function retrieves the host name of the system. It uses the gethostname() system call to get the
 * host name and stores it in a string. The maximum length of the host name is determined using the
 * sysconf(_SC_HOST_NAME_MAX) system call. The function dynamically allocates a buffer of size len and
 * passes it to gethostname() function. If the gethostname() call is successful, it returns the host name,
 * otherwise it throws a std::runtime_error with the error message obtained from strerror().
 *
 * @return The host name of the system.
 *
 * @throws std::runtime_error If the gethostname() call fails.
 */
std::string get_host_name () {
    const size_t len = sysconf(_SC_HOST_NAME_MAX) + 1; // +1 for the null-terminator
    std::vector<char> buffer(len);

    if (gethostname(buffer.data(), buffer.size()) != 0) {
        throw std::runtime_error("gethostname() failed: " + std::string(strerror(errno)));
    }

    return buffer.data();
}

/**
 * @brief Get the primary IP address of the specified interface.
 *
 * This function retrieves the primary IP address of the specified interface (eth0) by iterating through the list
 * of network interfaces on the system obtained using getifaddrs(). It checks for interfaces of type AF_INET (IPv4)
 * and with the name "eth0". Once a matching interface is found, it retrieves the IP address using inet_ntop() and
 * stores it in primaryIpAddress.
 *
 * @return The primary IP address of the specified interface.
 *
 * @throws std::runtime_error If the system call getifaddrs() fails to retrieve the interface information.
 *
 * @see getifaddrs()
 * @see inet_ntop()
 */
std::string get_interface_address () {
    struct ifaddrs *interfaces;
    std::string primaryIpAddress;

    if (getifaddrs(&interfaces) == -1) {
        throw std::runtime_error("Can't get the interface informations");
    } else {
        for (struct ifaddrs *temp = interfaces; temp != nullptr; temp = temp->ifa_next) {
            if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET && temp->ifa_name == std::string("eth0")) {
                char ip[INET_ADDRSTRLEN];

                void* tmpAddrPtr = &reinterpret_cast<struct sockaddr_in *>(temp->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, tmpAddrPtr, ip, INET_ADDRSTRLEN);
                primaryIpAddress = std::string(ip);
                break;
            }
        }
    }

    freeifaddrs(interfaces);
    return primaryIpAddress;
}

/**
 * Creates a new multicast socket.
 *
 * @param group_ip The IP address of the multicast group.
 * @return The socket file descriptor.
 * @throws std::runtime_error if an error occurs while creating or configuring the socket.
 */
int new_multicast_socket (const char *group_ip) {

    int sock = socket (AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error ("Failed to create socket");
    }

    int yes = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) < 0) {
        close (sock);
        throw std::runtime_error ("Setting SO_REUSEADDR failed");
    }

    struct ip_mreq mreq = {};
    mreq.imr_multiaddr.s_addr = inet_addr (group_ip);
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);

    if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        close (sock);
        throw std::runtime_error ("Adding IP_ADD_MEMBERSHIP failed");
    }

    return sock;
}

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

    int sock = new_multicast_socket (group_ip);

    nlohmann::ordered_json j;

    auto hostname = get_host_name();
    auto address = get_interface_address();

    j["id"] = hostname;
    j["address"] = address;
    j["active"] = true;
    j["architecture"] = "x86_64";
    j["provides"] = { "msmtpd", "video" };

    std::string message = j.dump(4);

    struct sockaddr_in group_addr = {};
    memset ((char *) &group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr (group_ip);
    group_addr.sin_port = htons (group_port);

    while (true) {
        if (sendto (sock, message.c_str(), message.size (), 0, reinterpret_cast<struct sockaddr *>(&group_addr), sizeof (group_addr)) <
            0) {
            perror ("Sending datagram message error");
            break;
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (500));
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

    int sock = new_multicast_socket (group_ip);

    struct sockaddr_in group_addr = {};
    memset (&group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    group_addr.sin_port = htons (group_port);

    if (bind (sock, reinterpret_cast<struct sockaddr *>(&group_addr), sizeof (group_addr)) < 0) {
        throw std::runtime_error ("Binding datagram socket error");
    }

    char buffer[1024];
    struct sockaddr_in src_addr = {};

    memset (&src_addr, 0, sizeof (src_addr));

    socklen_t src_addr_len = sizeof (src_addr);

    while (true) {
        if (ssize_t received = recvfrom (sock, buffer, sizeof (buffer), 0, reinterpret_cast<struct sockaddr *>(&src_addr), &src_addr_len); received < 0) {
            perror ("Receiving datagram message error");
            break;
        } else {
            buffer[received] = '\0'; {
                std::string source =
                        inet_ntoa (src_addr.sin_addr) + std::string (":") + std::to_string (ntohs (src_addr.sin_port));
                std::string message (buffer);
                std::lock_guard<std::mutex> lock (messages_mutex);

                if (!messages_map.contains (source)) {
                    // TODO: process new entry
                    std::cout << source << " acquired." << std::endl;
                }

                messages_map[source] = std::make_pair (message, get_timestamp());
            }

            print_messages ();
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

        auto now = get_timestamp();

        {
            std::lock_guard<std::mutex> lock (messages_mutex);

            for (auto it = messages_map.begin (); it != messages_map.end ();) {
                if (now - it->second.second > EXPIRY_MS) {
                    // process expired message
                    std::cout << it->first << " expired." << std::endl;
                    it = messages_map.erase (it);
                } else {
                    ++it;
                }
            }
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (250));
    }
}

/**
 * @file main.cpp
 * @brief This file contains the main function which creates and joins threads for transmitting, receiving, and expiration.
 */
int main () {
    std::thread multicastSender (transmit_thread, "224.1.1.1", 50000);
    std::thread multicastReceiver (receive_thread, "224.1.1.1", 50000);
    std::thread expiryScanner (expire_thread);

    multicastSender.join ();
    multicastReceiver.join ();
    expiryScanner.join ();

    return 0;
}

