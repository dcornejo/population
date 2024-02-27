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
//#include <ctime>
#include <mutex>
#include <chrono>

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
 * @brief Get current timestamp in milliseconds since epoch.
 *
 * This function returns the current timestamp in milliseconds since
 * 1st January 1970 (epoch) using the system clock. The resolution of
 * the timestamp depends on the system clock used and may vary.
 *
 * @return The current timestamp in milliseconds.
 */
std::int64_t get_timestamp () {

    std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
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
    std::string message = "{\"id\": 0}";

    struct sockaddr_in group_addr = {};
    memset ((char *) &group_addr, 0, sizeof (group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr (group_ip);
    group_addr.sin_port = htons (group_port);

    while (true) {
        if (sendto (sock, message.c_str (), message.size (), 0, (struct sockaddr *) &group_addr, sizeof (group_addr)) <
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

    if (bind (sock, (struct sockaddr *) &group_addr, sizeof (group_addr)) < 0) {
        throw std::runtime_error ("Binding datagram socket error");
    }

    char buffer[1024];
    struct sockaddr_in src_addr = {};

    memset (&src_addr, 0, sizeof (src_addr));

    socklen_t src_addr_len = sizeof (src_addr);

    while (true) {
        ssize_t received = recvfrom (sock, buffer, sizeof (buffer), 0, (struct sockaddr *) &src_addr, &src_addr_len);

        if (received < 0) {
            perror ("Receiving datagram message error");
            break;
        } else {
            buffer[received] = '\0';

            std::string source =
                    inet_ntoa (src_addr.sin_addr) + std::string (":") + std::to_string (ntohs (src_addr.sin_port));
            std::string message (buffer);

            {
                std::lock_guard<std::mutex> lock (messages_mutex);

                if (messages_map.find (source) == messages_map.end ()) {
                    // TODO: process new entry
                    std::cout << source << " acquired." << std::endl;
                }

                messages_map[source] = std::make_pair (message, get_timestamp());
            }

            //print_messages ();
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

const std::int64_t EXPIRY_MS = 1000;

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
