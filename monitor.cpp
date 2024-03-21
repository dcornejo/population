
#include <string>
#include <chrono>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

#include "monitor.h"
#include "comms.h"

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
            std::chrono::system_clock::now ();

    const auto duration = now.time_since_epoch ();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ();

    return millis;
}

/**
 * @brief Print the timestamp in the format HH:MM:SS.SSS
 *
 * This function takes a timestamp in milliseconds and calculates the hours, minutes, seconds, and milliseconds.
 * Then it prints the timestamp in the format HH:MM:SS.SSS to the standard output.
 *
 * @param timestamp The timestamp in milliseconds
 */
void print_timestamp (std::uint64_t timestamp) {
    // Calculate the hours, minutes, seconds and milliseconds

    std::uint64_t hours = ((timestamp % 86400000) / 3600000);
    timestamp %= 3600000;
    std::uint64_t minutes = timestamp / 60000;
    timestamp %= 60000;
    std::uint64_t seconds = timestamp / 1000;
    std::uint64_t milliseconds = timestamp % 1000;

    // Print the timestamp
    std::cout << std::setw (2) << std::setfill ('0') << hours << ":"
              << std::setw (2) << std::setfill ('0') << minutes << ":"
              << std::setw (2) << std::setfill ('0') << seconds << "."
              << std::setw (3) << std::setfill ('0') << milliseconds;
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
    auto ts = get_timestamp ();
    auto status = PARTICIPANT_EXISTS;

    const std::string id = j["id"].get<std::string> ();
    const std::string address = j["address"].get<std::string> ();
    const std::string architecture = j["architecture"].get<std::string> ();

    std::lock_guard lock (participant_mutex);

    if (participant_map.contains (id)) {
        // updating existing entry

        auto w = participant_map[id];

        w["last_seen"] = ts;
        participant_map[id] = w;

    } else {
        // adding new entry

        j["first_seen"] = ts;
        j["last_seen"] = ts;
        participant_map[id] = j;

        print_timestamp (ts);
        std::cout << ": " << id << " online " << std::endl;

        send_update (address, 1, architecture);

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
 *       std::map<std::string, json> used to store participant details.
 *
 * @note This method assumes the existence of the
 */
int expire_participants () {
    const auto current_timestamp = get_timestamp ();

    for (auto it = participant_map.begin (); it != participant_map.end ();) {

        json p = it->second;

        std::lock_guard lock (participant_mutex);

        const auto last_seen = p["last_seen"].get<std::uint64_t> ();

        if (const std::uint64_t age = current_timestamp - last_seen; age > 600) {

            std::string id = p["id"].get<std::string> ();
            std::string address = p["address"].get<std::string> ();
            std::string architecture = p["architecture"].get<std::string> ();

            print_timestamp (current_timestamp);
            std::cout  << ": " << id << " offline " << std::endl;

            send_update (address, 1, architecture);
            
            it = participant_map.erase (it);
        } else {
            ++it;
        }
    }

    return 0;
}


