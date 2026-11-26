#include "astronomy/fn/Sky.hpp"

#include "astronomy/constant/Sky.hpp"
#include "astronomy/fn/DeepTime.hpp"
#include "astronomy/fn/Frame.hpp"
#include "astronomy/fn/Orbit.hpp"

namespace derivean::sim {

Sky skyAt(const Instant& instant) {
    Sky sky;
    sky.deep = deepTimeAt(instant.age);
    const MeanElements elements = meanElementsAt(instant, sky.deep);
    const Vec3 moon = moonFromEarth(elements, sky.deep);
    // The Earth swings about the pair's barycentre by 1/82 of the Moon's distance (4 700 km),
    // which moves the Sun by up to 6".
    const Vec3 sun = sunFromBarycentre(elements) + moon * (MoonGM / (EarthGM + MoonGM));

    sky.precession = elements.precession;
    sky.rotation = rotationAt(instant, sky.deep);
    sky.meanSunLongitude = elements.sunLongitude + elements.precession;
    sky.sun = eclipticToEarth(sun, sky.precession, sky.rotation);
    sky.moon = eclipticToEarth(moon, sky.precession, sky.rotation);
    sky.sunInertial = eclipticToInertial(sun);
    sky.moonInertial = eclipticToInertial(moon);
    return sky;
}

} // namespace derivean::sim
