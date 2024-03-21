#include "monitor.h"

#include <string>
#include <chrono>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::map<std::string, json> participant_map;
std::mutex participant_mutex;

/**
 * @brief Prints the timestamp in hours, minutes, seconds and milliseconds
 *
 * This function takes a timestamp in milliseconds since epoch and prints it in a human readable
 * format of hours, minutes, seconds and milliseconds.
 *
 * @return The current timestamp in milliseconds.
 */
std::uint64_t get_timestamp () {
    const std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

    const auto duration = now.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

void print_timestamp(std::uint64_t timestamp) {
    // Calculate the hours, minutes, seconds and milliseconds

    std::uint64_t hours = ((timestamp % 86400000) / 3600000);
    timestamp %= 3600000;
    std::uint64_t minutes = timestamp / 60000;
    timestamp %= 60000;
    std::uint64_t seconds = timestamp / 1000;
    std::uint64_t milliseconds = timestamp % 1000;

    // Print the timestamp
    std::cout << std::setw(2) << std::setfill('0') << hours << ":"
              << std::setw(2) << std::setfill('0') << minutes << ":"
              << std::setw(2) << std::setfill('0') << seconds << "."
              << std::setw(3) << std::setfill('0') << milliseconds;
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
ParticipantStatus report_participant (json &j) {
    auto ts = get_timestamp();
    auto status = PARTICIPANT_EXISTS;

    const std::string id = j["id"];

    std::lock_guard lock(participant_mutex);

    if (participant_map.contains(id)) {
        // updating existing entry

        auto w = participant_map[id];

        w["last_seen"] = ts;
        participant_map[id] = w;

    } else {
        // adding new entry

        j["first_seen"] = ts;;
        j["last_seen"] = ts;
        participant_map[id] = j;

        print_timestamp(ts);
        std::cout << ": online " << id <<std::endl;

        status = PARTICIPANT_ADDED;
    }

//    std::cout << participant_map[id].dump(2) << "\n" << std::endl;

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
 *       std::map<std::string, json> used to store participant details.
 *
 * @note This method assumes the existence of the
 */
void check_participants () {
    const auto current_timestamp = get_timestamp();

    for (auto it = participant_map.begin(); it != participant_map.end();) {

        const auto last_seen = it->second["last_seen"].get<std::uint64_t>();
        auto id = it->second["id"];

        std::lock_guard lock(participant_mutex);

        if (const std::uint64_t age = current_timestamp - last_seen; age > 600) {
            std::cout << current_timestamp << ": offline " << id << std::endl;
            it = participant_map.erase(it);
        } else {
            ++it;
        }
    }
}

