#pragma once

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>

// `--name value` pairs after the command word.
struct Arguments {
    std::map<std::string, std::string> values;

    Arguments(int count, char** words) {
        for (int i = 2; i + 1 < count; i += 2) {
            const std::string name = words[i];
            if (name.rfind("--", 0) == 0) values[name.substr(2)] = words[i + 1];
        }
    }

    double number(const std::string& name, double fallback) const {
        const auto found = values.find(name);
        return found == values.end() ? fallback : std::strtod(found->second.c_str(), nullptr);
    }

    uint64_t seed() const {
        const auto found = values.find("seed");
        return found == values.end() ? 42 : std::strtoull(found->second.c_str(), nullptr, 10);
    }

    std::string text(const std::string& name, const std::string& fallback) const {
        const auto found = values.find(name);
        return found == values.end() ? fallback : found->second;
    }
};

int planetCommand(const Arguments& arguments);
int mapCommand(const Arguments& arguments);
int chartCommand(const Arguments& arguments);
int probeCommand(const Arguments& arguments);
int sliceCommand(const Arguments& arguments);
int findCommand(const Arguments& arguments);
int groundCommand(const Arguments& arguments);
int graphCommand(const Arguments& arguments);
int atmosphereCommand(const Arguments& arguments);
int hazeCommand(const Arguments& arguments);
int snowCommand(const Arguments& arguments);
int measureCommand(const std::string& what, const Arguments& arguments);
int inspectCommand(const Arguments& arguments);
int scoutCommand(const Arguments& arguments);
int viewCommand(const Arguments& arguments);
int reliefCommand(const Arguments& arguments);
int blockCommand(const Arguments& arguments);
int aeolianCommand(const Arguments& arguments);
int coastCommand(const Arguments& arguments);
int groundwaterCommand(const Arguments& arguments);
int volcanicCommand(const Arguments& arguments);
int iceSheetCommand(const Arguments& arguments);
int interventionCommand(const std::string& scenario, const Arguments& arguments);
