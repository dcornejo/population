
#ifndef CONFIG_H
#define CONFIG_H

#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern json configuration;

void load_configuration ();

#endif //CONFIG_H
