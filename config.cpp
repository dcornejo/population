#include "config.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "utilities.h"

using json = nlohmann::json;

json configuration;

void load_configuration () {

    std::ifstream f("../config.json");
    json data = json::parse(f);

    configuration = data;

    if (!configuration.contains("id")) {
        configuration["id"] = get_host_name();
    }

}
