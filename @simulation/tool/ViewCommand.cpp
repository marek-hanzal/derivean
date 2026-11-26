#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Scene.hpp"
#include "diagnostics/fn/View.hpp"
#include "measure/constant/Angle.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra view --x m --z m [--above m | --y m] [--heading deg | --yaw deg | --to-x m --to-z m] [--pitch deg] [--fov deg]
//            [--sun-azimuth deg] [--sun-elevation deg] [--visibility m] [--site m] [--local m] [--region m]
//            [--snow 0] [--width px] [--height px] [--out directory] [--name tag]
// A perspective picture straight from the simulation's tiles. Heading is a compass bearing (0 north,
// 90 east); --yaw takes the game's angle instead (0 north, positive turning west). --to-x/--to-z aim
// the camera at the ground there. --site, --local and --region are the distances from the eye within
// which each level is built; 0 leaves a level out. --snow 0 shows the ground under the lasting snow
// that the tile's climate implies; glacier ice the tiles hold is always drawn.
int viewCommand(const Arguments& arguments) {
    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const double planetSeconds = lap();

    Camera camera;
    camera.eye = {arguments.number("x", 0), 0, arguments.number("z", 0)};
    camera.width = int(arguments.number("width", 1280)), camera.height = int(arguments.number("height", 800));
    camera.fov = arguments.number("fov", 60) / DegreesPerRadian;
    camera.heading = arguments.values.count("yaw") ? -arguments.number("yaw", 0) / DegreesPerRadian : arguments.number("heading", 0) / DegreesPerRadian;
    camera.pitch = arguments.number("pitch", 0) / DegreesPerRadian;
    const bool aimed = arguments.values.count("to-x") && arguments.values.count("to-z");
    const Vec2 target{arguments.number("to-x", 0), arguments.number("to-z", 0)};
    if (aimed) camera.heading = std::atan2(target.x - camera.eye.x, -(target.z - camera.eye.z));

    SceneReach reach;
    reach.site = arguments.number("site", reach.site), reach.local = arguments.number("local", reach.local), reach.region = arguments.number("region", reach.region);
    reach.snow = arguments.number("snow", 1) > 0;
    SceneCount count;
    const Scene scene = gatherScene(world, camera, reach, &count);
    const double sceneSeconds = lap();

    const double ground = sceneTop(scene, camera.eye.x, camera.eye.z);
    camera.eye.y = arguments.values.count("y") ? arguments.number("y", 0) : ground + arguments.number("above", 2);
    if (aimed) camera.pitch = std::atan2(sceneTop(scene, target.x, target.z) - camera.eye.y, length(target - Vec2{camera.eye.x, camera.eye.z}));

    Light light;
    light.azimuth = arguments.values.count("sun-azimuth") ? arguments.number("sun-azimuth", 0) / DegreesPerRadian : camera.heading - 100 / DegreesPerRadian;
    light.elevation = arguments.number("sun-elevation", 32) / DegreesPerRadian;
    light.visibility = arguments.number("visibility", light.visibility);

    const std::vector<uint8_t> image = renderView(scene, camera, light, world.workers);
    const std::string path = arguments.text("out", ".") + "/view-" + arguments.text("name", "shot") + ".png";
    writePng(path, camera.width, camera.height, image);

    std::printf("view  seed %llu  age %.3g yr  planet %.1f s  tiles: %d region, %d local, %d site in %.1f s  picture %.1f s\n",
                static_cast<unsigned long long>(arguments.seed()), world.options.age, planetSeconds, count.region, count.local, count.site, sceneSeconds, lap());
    std::printf("eye X %.0f Y %.1f Z %.0f (%.1f m above the ground)  heading %.1f  pitch %.1f  fov %.0f  sun from %.0f at %.0f deg\n", camera.eye.x, camera.eye.y,
                camera.eye.z, camera.eye.y - ground, camera.heading * DegreesPerRadian, camera.pitch * DegreesPerRadian, camera.fov * DegreesPerRadian,
                light.azimuth * DegreesPerRadian, light.elevation * DegreesPerRadian);
    const ViewHit hit = centreHit(scene, camera);
    if (hit.found) std::printf("centre of the picture: X %.0f Y %.1f Z %.0f, %.0f m away, drawn from %.0f m cells\n", hit.at.x, hit.at.y, hit.at.z, hit.distance, hit.cell);
    else std::printf("centre of the picture: sky\n");
    std::printf("%s\n", path.c_str());
    return 0;
}
