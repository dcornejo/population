//
// Created by dave on 3/15/2024.
//

#ifndef UTILITIES_H
#define UTILITIES_H

#include <map>
#include <mutex>
#include <string>
#include <sys/utsname.h>


extern std::map<std::string, std::pair<std::string, time_t>> messages_map;
extern std::mutex messages_mutex;

void print_messages ();

std::string get_host_name ();
std::string get_interface_address ();
int new_multicast_socket (const char *group_ip);

class system_info {
public:
    std::string sysname;
    std::string nodename;
    std::string release;
    std::string version;
    std::string machine;

    system_info () {
        struct ::utsname info;

        if (::uname(&info) < 0) {
            throw std::runtime_error("Failed to get system information");
        }
        else {
            sysname = info.sysname;
            nodename = info.nodename;
            release = info.release;
            version = info.version;
            machine = info.machine;
        }
    }
};

#endif //UTILITIES_H
