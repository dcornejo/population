//
// Created by dave on 3/4/2024.
//

#ifndef HOSTMON_MONITOR_H
#define HOSTMON_MONITOR_H

#include <string>
#include <vector>

class participant;

enum ParticipantStatus {
    PEXISTS,
    PADDED
};


class participant {
private:
    // the time we first saw this participant
    std::uint64_t first_seen;

    // the time we last saw this participant
    std::uint64_t seen;

    // an identifier for this host (uses hostname)
    std::string id;
    // the ip address of the participant
    std::string address;

    // whether the participant is currently an active provider
    bool active;
    // the participants reported CPU architecture
    std::string architecture;
    // list of services provided by this participant
    std::vector<std::string> provides;

public:
    participant() : first_seen(0), seen(0), active(false) {}


    [[nodiscard]] std::string get_id () const {
        return id;
    }

    void set_id (const std::string &new_id) {
        id = new_id;
    }

    [[nodiscard]] std::string get_address () const {
        return address;
    }

    void set_address (const std::string &new_address) {
        address = new_address;
    }

    [[nodiscard]] bool is_active () const {
        return active;
    }

    void set_active (bool new_active) {
        active = new_active;
    }

    [[nodiscard]] std::string get_architecture () const {
        return architecture;
    }

    void set_architecture (const std::string &new_architecture) {
        architecture = new_architecture;
    }

    [[nodiscard]] std::vector<std::string> get_provides () const {
        return provides;
    }

    void set_provides (const std::vector<std::string> &new_provides) {
        provides = new_provides;
    }

    [[nodiscard]] uint64_t get_first_seen () const {
        return first_seen;
    }

    void set_first_seen (const std::uint64_t &new_first_seen) {
        first_seen = new_first_seen;
    }

    [[nodiscard]] uint64_t get_last_seen () const {
        return seen;
    }

    void set_last_seen (const std::uint64_t &new_last_seen) {
        seen = new_last_seen;
    }
};

std::uint64_t get_timestamp ();
ParticipantStatus report_participant (const participant &p);

#endif //HOSTMON_MONITOR_H
