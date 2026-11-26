#include "Arguments.hpp"

#include <cstdio>
#include <string>

int main(int count, char** words) {
    const std::string command = count > 1 ? words[1] : "";
    const Arguments arguments(count, words);
    if (command == "planet") return planetCommand(arguments);
    if (command == "map") return mapCommand(arguments);
    if (command == "chart") return chartCommand(arguments);
    if (command == "probe") return probeCommand(arguments);
    if (command == "slice") return sliceCommand(arguments);
    if (command == "find") return findCommand(arguments);
    if (command == "ground") return groundCommand(arguments);
    if (command == "atmosphere") return atmosphereCommand(arguments);
    if (command == "snow") return snowCommand(arguments);
    if (command == "inspect") return inspectCommand(arguments);
    if (command == "scout") return scoutCommand(arguments);
    if (command == "view") return viewCommand(arguments);
    if (command == "relief") return reliefCommand(arguments);
    if (command == "block") return blockCommand(arguments);
    if (command == "aeolian") return aeolianCommand(arguments);
    if (command == "coast") return coastCommand(arguments);
    if (command == "groundwater") return groundwaterCommand(arguments);
    if (command == "volcanic") return volcanicCommand(arguments);
    if (command == "measure") return measureCommand(count > 2 ? words[2] : "", Arguments(count - 1, words + 1));
    if (command == "intervention") return interventionCommand(count > 2 ? words[2] : "", Arguments(count - 1, words + 1));
    std::fprintf(stderr, "usage: terra <planet> [--seed n] [--age years] [--out directory]\n");
    return 2;
}
