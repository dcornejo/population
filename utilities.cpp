//
// Created by dave on 3/15/2024.
//

#include "utilities.h"

#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <map>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>

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
    std::lock_guard<std::mutex> lock(messages_mutex);

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
    }
    else {
        for (struct ifaddrs *temp = interfaces; temp != nullptr; temp = temp->ifa_next) {
            if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET && temp->ifa_name == std::string("eth0")) {
                char ip[INET_ADDRSTRLEN];

                void *tmpAddrPtr = &reinterpret_cast<struct sockaddr_in *>(temp->ifa_addr)->sin_addr;
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
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        close(sock);
        throw std::runtime_error("Setting SO_REUSEADDR failed");
    }

    struct ip_mreq mreq = {};
    mreq.imr_multiaddr.s_addr = inet_addr(group_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        close(sock);
        throw std::runtime_error("Adding IP_ADD_MEMBERSHIP failed");
    }

    return sock;
}

