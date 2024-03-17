#include "monitor.h"

#include <string>
#include <chrono>
#include <iostream>
#include <map>
#include <vector>

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
