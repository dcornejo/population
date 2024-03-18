
#include "utilities.h"

#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
 * TODO: find a way to find the actual primary interface name/ip
 *
 * @return The primary IP address of the specified interface.
 *
 * @throws std::runtime_error If the system call getifaddrs() fails to retrieve the interface information.
 *
 * @see getifaddrs()
 * @see inet_ntop()
 */
std::string get_interface_address () {
    ifaddrs *interfaces;
    std::string primaryIpAddress;

    if (getifaddrs(&interfaces) == -1) {
        throw std::runtime_error("Can't get the interface informations");
    }

    for (const ifaddrs *temp = interfaces; temp != nullptr; temp = temp->ifa_next) {
        if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET && temp->ifa_name == std::string("eth0")) {
            char ip[INET_ADDRSTRLEN];

            const void *tmpAddrPtr = &reinterpret_cast<struct sockaddr_in *>(temp->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ip, INET_ADDRSTRLEN);
            primaryIpAddress = std::string(ip);
            break;
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
    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    constexpr int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        close(sock);
        throw std::runtime_error("Setting SO_REUSEADDR failed");
    }

    ip_mreq mreq = {};
    mreq.imr_multiaddr.s_addr = inet_addr(group_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        close(sock);
        throw std::runtime_error("Adding IP_ADD_MEMBERSHIP failed");
    }

    return sock;
}


