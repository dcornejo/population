#include "config.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "utilities.h"

using json = nlohmann::json;

/*
 * this is a global json object containing the configuration read in
 */
json configuration;

/**
 * @brief Loads the configuration from the config.json file.
 *
 * This function reads the contents of the config.json file and parses it into
 * a json object. The configuration data is then stored in the global variable
 * 'configuration'.
 *
 * If the 'id' field is not present in the configuration, it will be populated
 * with the value returned by the get_host_name() function.
 *
 * @note This function assumes that the config.json file is present in the
 *       relative path "../config.json".
 *
 * @throws std::ifstream::failure If there is an error while opening or reading
 *                                the config.json file.
 */
void load_configuration () {

    std::ifstream f("../config.json");
    json data = json::parse(f);

    configuration = data;

    if (!configuration.contains("id")) {
        configuration["id"] = get_host_name();
    }

}
