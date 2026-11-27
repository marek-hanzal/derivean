#include "Arguments.hpp"

#include <cstdio>
#include <string>

// The globe history (S3b) until S3e makes it the planet.
int globeCommand(const Arguments& arguments);

int main(int count, char** words) {
    const std::string command = count > 1 ? words[1] : "";
    const Arguments arguments(count, words);
    if (command == "planet") return planetCommand(arguments);
    if (command == "globe") return globeCommand(arguments);
    if (command == "map") return mapCommand(arguments);
    if (command == "chart") return chartCommand(arguments);
    if (command == "probe") return probeCommand(arguments);
    if (command == "slice") return sliceCommand(arguments);
    if (command == "find") return findCommand(arguments);
    if (command == "ground") return groundCommand(arguments);
    if (command == "graph") return graphCommand(arguments);
    if (command == "atmosphere") return atmosphereCommand(arguments);
    if (command == "haze") return hazeCommand(arguments);
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
    if (command == "icesheet") return iceSheetCommand(arguments);
    if (command == "approx") return approxCommand(arguments);
    if (command == "approx-check") return approxCheckCommand(arguments);
    if (command == "measure") return measureCommand(count > 2 ? words[2] : "", Arguments(count - 1, words + 1));
    if (command == "intervention") return interventionCommand(count > 2 ? words[2] : "", Arguments(count - 1, words + 1));
    std::fprintf(stderr, "usage: terra <planet> [--seed n] [--age years] [--out directory]\n");
    return 2;
}
