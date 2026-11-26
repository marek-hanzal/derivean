#include "Volcanic.hpp"

// terra volcanic --scene shield|cone|valley|caldera|worn|cinder|summit [--volume km3] [--episodes n] [--quiet steps] [--paroxysm km3] [--out directory]
// terra volcanic --x m --z m [--reach m] [--out directory] [--name tag]
// terra volcanic --survey 1 [--seed n] [--age years]
int volcanicCommand(const Arguments& arguments) {
    if (arguments.values.count("scene")) return volcanicScene(arguments);
    if (arguments.values.count("survey")) return volcanicSurvey(arguments);
    return volcanicWorld(arguments);
}
