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

/*
 * map to store active peers
 */
std::map<std::string, std::pair<std::string, time_t>> messages_map;

void print_messages () {
    for (auto &it: messages_map) {
        std::cout << "Source: " << it.first << "\n"
                  << "Message: " << it.second.first << "\n"
                  << "Time: " << it.second.second << "\n\n";
    }
}

/**
 * @brief Creates a new multicast socket with the given group IP.
 *
 * This function creates a new multicast socket that can be used to send and receive
 * multicast datagrams on the network. The group IP specifies the IP address of the
 * multicast group to join.
 *
 * @param group_ip The IP address of the multicast group to join.
 * @return The file descriptor of the created multicast socket on success, -1 on failure.
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

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr (group_ip);
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);

    if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof (mreq)) < 0) {
        close (sock);
        throw std::runtime_error ("Adding IP_ADD_MEMBERSHIP failed");
    }

    return sock;
}

/**
 * @brief Transmits a multicast message to the specified group IP and port.
 *
 * This function creates a new multicast socket, sets up the group IP and port, and continuously
 * sends a message to the multicast group at a regular interval.
 *
 * @param group_ip The IP address of the multicast group to transmit to.
 * @param group_port The port number of the multicast group to transmit to.
 */
void transmit_thread (const char *group_ip, unsigned short group_port) {

    int sock = new_multicast_socket (group_ip);
    std::string message = "Hello Multicast Group";

    struct sockaddr_in group_addr;
    memset ((char *) &group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr (group_ip);
    group_addr.sin_port = htons (group_port);

    while (true) {
        if (sendto (sock, message.c_str (), message.size (), 0, (struct sockaddr *) &group_addr, sizeof (group_addr)) <
            0) {
            perror ("Sending datagram message error");
        }
        std::this_thread::sleep_for (std::chrono::milliseconds (500));
    }
}

/**
 * @brief Receive thread function to listen for incoming messages on a multicast group.
 *
 * This function creates a socket and joins a multicast group specified by the given IP address and port number.
 * It continuously receives packets from the group and processes them until the thread is terminated.
 *
 * @param group_ip The IP address of the multicast group.
 * @param group_port The port number of the multicast group.
 */
void receive_thread (const char *group_ip, unsigned short group_port) {

    int sock = new_multicast_socket (group_ip);

    struct sockaddr_in group_addr;
    memset (&group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    group_addr.sin_port = htons (group_port);

    if (bind (sock, (struct sockaddr *) &group_addr, sizeof (group_addr)) < 0) {
        throw std::runtime_error ("Binding datagram socket error");
    }

    char buffer[1024];
    struct sockaddr_in src_addr;

    memset (&src_addr, 0, sizeof (src_addr));

    socklen_t src_addr_len = sizeof (src_addr);

    while (true) {
        ssize_t received = recvfrom (sock, buffer, sizeof (buffer), 0, (struct sockaddr *) &src_addr, &src_addr_len);

        if (received < 0) {
            perror ("Receiving datagram message error");
        } else {
            buffer[received] = '\0';

            std::string source =
                    inet_ntoa (src_addr.sin_addr) + std::string (":") + std::to_string (ntohs (src_addr.sin_port));
            std::string message (buffer);

            if (messages_map.find (source) == messages_map.end ()) {
                // TODO: process new entry
                std::cout << "NEW\n";
            }
            messages_map[source] = std::make_pair (message, time (nullptr));

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
void expire_thread () {
    while (true) {
        time_t now = time (nullptr);
        for (auto it = messages_map.begin (); it != messages_map.end ();) {
            if (now - it->second.second > 5) {
                it = messages_map.erase (it);
            } else {
                ++it;
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
