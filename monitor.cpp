#include "monitor.h"

#include <string>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>

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

    // TODO: we need to combine service and state

    // whether the participant is currently an active provider
    bool active;
    // the participants reported CPU architecture
    std::string architecture;
    // list of services provided by this participant
    std::vector<std::string> provides;

public:
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

std::map<std::string, participant> participant_map;

/**
 * @brief Get current timestamp in milliseconds since epoch.
 *
 * This function returns the current timestamp in milliseconds since
 * 1st January 1970 (epoch) using the system clock. The resolution of
 * the timestamp depends on the system clock used and may vary.
 *
 * @return The current timestamp in milliseconds.
 */
std::uint64_t get_timestamp () {
    std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}


ParticipantStatus report_participant (const participant &p) {
    auto it = participant_map.find(p.get_id());
    auto ts = get_timestamp();

    if (it != participant_map.end()) {
        // Participant exists, so we update 'seen' and return PEXISTS

        it->second.set_last_seen(ts);

        std::cout << "saw " << p.get_id() << " at " << ts << std::endl;

        return PEXISTS;
    }
    else {
        // New participant, so we add to the map and return PADDED

        participant new_participant = p;

        new_participant.set_last_seen(ts);
        new_participant.set_first_seen(ts);

        participant_map[new_participant.get_id()] = new_participant;

        std::cout << "new " << p.get_id() << " at " << ts << std::endl;

        return PADDED;
    }
}
