#include "monitor.h"

#include <string>
#include <chrono>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

std::map<std::string, ordered_json> participant_map;
std::mutex participant_mutex;

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

/**
 * @brief Reports the participant to the monitoring system.
 *
 * This function is used to report a participant to the monitoring system. It updates
 * the participant's last seen timestamp and adds a new participant if it doesn't exist
 * in the participant map. The function also prints debugging information to the console.
 *
 * @param j The JSON object representing the participant.
 * @return The status of the participant after reporting (PARTICIPANT_EXISTS or PARTICIPANT_ADDED).
 */
ParticipantStatus report_participant (nlohmann::ordered_json &j) {
    auto ts = get_timestamp();
    auto status = PARTICIPANT_EXISTS;

    std::string id = j["id"];

    std::lock_guard<std::mutex> lock(participant_mutex);

    if (participant_map.contains(id)) {
        auto w = participant_map[id];

        // updating existing entry

        w["last_seen"] = ts;
        participant_map[id] = w;

        // std::cout << ts << ": updating " << id <<std::endl;
    } else {
        // adding new entry

        j["first_seen"] = ts;;
        j["last_seen"] = ts;
        participant_map[id] = j;

        std::cout << ts << ": online " << id <<std::endl;

        status = PARTICIPANT_ADDED;
    }

    return status;
}

/**
 * @brief Check the participants for stale entries and remove them.
 *
 * This function iterates over the participant_map and checks the age
 * of each participant based on the current timestamp obtained from
 * the get_timestamp() function. If a participant's age is greater than
 * 1000 milliseconds, it is considered stale and removed from the map.
 * The details of the stale entry are printed to the console.
 *
 * @note This method assumes the existence of a participant_map, which is a
 *       std::map<std::string, ordered_json> used to store participant details.
 *
 * @note This method assumes the existence of the*/
void check_participants () {
    auto current_timestamp = get_timestamp();

    for (auto it = participant_map.begin(); it != participant_map.end();) {

        auto last_seen = it->second["last_seen"].get<std::uint64_t>();
        auto id = it->second["id"];

        std::uint64_t age = current_timestamp - last_seen;

        if (age > 1000) {
            std::cout << current_timestamp << ": offline " << id << std::endl;
            it = participant_map.erase(it);
        } else {
            ++it;
        }
    }
}

