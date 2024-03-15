
#include <string>
#include <chrono>
#include <map>
#include <vector>

class participant {
private:
    // the time we first saw this participant
    std::chrono::time_point<std::chrono::system_clock> first_seen;

    // the time we last saw this participant
    std::chrono::time_point<std::chrono::system_clock> seen;

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
std::int64_t get_timestamp () {

    std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

