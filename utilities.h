//
// Created by dave on 3/15/2024.
//

#ifndef UTILITIES_H
#define UTILITIES_H

#include <mutex>
#include <string>
#include <sys/utsname.h>


std::string get_host_name ();
std::string get_interface_address ();
int new_multicast_socket (const char *group_ip);

/**
 * @class system_info
 * @brief Represents system information
 *
 * The system_info class provides information about the underlying
 * system, such as the system name, node name, release version, and machine type.
 */
class system_info {
public:
    std::string sysname;
    std::string nodename;
    std::string release;
    std::string version;
    std::string machine;

    system_info () {
        utsname info = {};

        if (uname(&info) < 0) {
            throw std::runtime_error("Failed to get system information");
        }

        sysname = info.sysname;
        nodename = info.nodename;
        release = info.release;
        version = info.version;
        machine = info.machine;
    }
};

#endif //UTILITIES_H
