#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>

int setup_multicast_socket (const char *group_ip) {

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

void transmit_thread (const char *group_ip, unsigned short group_port) {

    int sock = setup_multicast_socket (group_ip);
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
        std::this_thread::sleep_for (std::chrono::milliseconds (1000));
    }
}

std::map<std::string, std::pair<std::string, time_t>> messages_map;

void print_messages () {
    for (auto &it: messages_map) {
        std::cout << "Source: " << it.first << "\n"
                  << "Message: " << it.second.first << "\n"
                  << "Time: " << it.second.second << "\n\n";
    }
}

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
        std::this_thread::sleep_for (std::chrono::milliseconds (1000));
    }
}

void receive_thread (const char *group_ip, unsigned short group_port) {

    int sock = setup_multicast_socket (group_ip);

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
        int received = recvfrom (sock, buffer, sizeof (buffer), 0, (struct sockaddr *) &src_addr, &src_addr_len);

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

int main () {
    std::thread multicastSender (transmit_thread, "224.1.1.1", 1900);
    std::thread multicastReceiver (receive_thread, "224.1.1.1", 1900);
    std::thread expiryScanner (expire_thread);

    multicastSender.join ();
    multicastReceiver.join ();
    expiryScanner.join ();

    return 0;
}
