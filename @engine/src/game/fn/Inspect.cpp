#include "game/type/Game.hpp"
#include "inspect/fn/Inspect.hpp"
#include "ground/type/Clast.hpp"
#include "relief/type/Form.hpp"
#include "simulation/fn/Query.hpp"

namespace derivean {
namespace {

constexpr const char* LevelNames[]{"none", "region", "local", "site", "ground"};
constexpr const char* BandNames[]{"Low", "Middle", "High"};

// The simulation's full record blocks until the site and ground tiles its column reads exist, so it
// is asked only where they already do.
bool surveyed(const sim::World& world, sim::Vec2 place) {
    return sim::readyAround(world, place, sim::FormMargin + sim::ExposureLattice + sim::ExposureReach + 1.0, sim::Level::Site) &&
           sim::readyAround(world, place, std::max(sim::FormMargin + 1.5, sim::ClastReach) + 1.0, sim::Level::Ground);
}

void describeWeather(Inspection& record, const Game& game, sim::Vec2 place, double ground) {
    const sim::WeatherReport report = sim::inspectWeather(*game.weather.seen.air, place, ground, game.calendar);
    const auto add = [&](std::string key, std::variant<double, std::string> value) { record.emplace_back("Weather / " + std::move(key), std::move(value)); };
    add("Why", report.cause);
    add("Air temperature °C", report.weather.temperature);
    add("Dew point °C", report.dewPoint);
    add("Relative humidity", report.weather.humidity);
    add("Pressure Pa", report.pressure);
    add("Rain mm/h", report.weather.rain);
    add("Snow mm/h", report.weather.snow);
    add("Ash fallen kg/m²", report.ash.mass);
    add("Ash falling kg/m²/s", report.ash.rate);
    add("Veil optical depth", report.aerosolDepth);
    add("Haze optical depth dust, salt, sulphate, organics", std::to_string(report.hazeDepth[0]) + ", " + std::to_string(report.hazeDepth[1]) + ", " + std::to_string(report.hazeDepth[2]) + ", " + std::to_string(report.hazeDepth[3]));
    add("Visibility m", report.visibility);
    add("Snow lying kg/m²", report.weather.snowpack);
    add("Snow line m above the sea", report.snowLine);
    add("Snow is kept", std::string("per weather cell at a ladder of heights; every height gets the cell's precipitation, without wind drift, aspect or avalanches"));
    add("Wind east m/s", report.weather.wind.x);
    add("Wind south m/s", report.weather.wind.z);
    add("Ascent m/s", report.verticalSpeed);
    add("Cloud cover", report.clouds.cover);
    add("Cloud genus", std::string(sim::genusName(report.clouds.genus)));
    add("Cloud optical depth", report.clouds.opticalDepth);
    add("Sunlight turned away", report.clouds.shade);
    add("Updrafts reach m", report.clouds.convectiveTop);
    add("Lapse °C/km", report.profile.lapse * 1000.0);
    add("Condensation level m", report.profile.condensationLevel);
    add("Convection", report.profile.convection);
    for (size_t band = 0; band < sim::AirBands; ++band) {
        const sim::CloudLayer& layer = report.clouds.layer[band];
        const std::string name = std::string(BandNames[band]) + " band ";
        add(name + "top m", report.profile.height[band + 1] + report.column.ground);
        add(name + "temperature °C", report.profile.celsius[band]);
        add(name + "humidity", report.humidity[band]);
        add(name + "vapour kg/m²", report.column.vapour[band]);
        add(name + "cloud water kg/m²", layer.liquidPath);
        add(name + "cloud ice kg/m²", layer.icePath);
        add(name + "falling kg/m²", report.column.falling[band]);
        add(name + "cloud cover", layer.cover);
        add(name + "cloud base m", layer.base);
        add(name + "cloud top m", layer.top);
        add(name + "cloud genus", std::string(sim::genusName(layer.genus)));
        add(name + "wind east m/s", report.wind[band].x);
        add(name + "wind south m/s", report.wind[band].z);
    }
    const sim::WaterLedger& ledger = report.ledger;
    add("Planet water kg", sim::total(ledger));
    add("Planet ocean kg", ledger.ocean);
    add("Planet vapour kg", ledger.vapour);
    add("Planet cloud water kg", ledger.cloudLiquid);
    add("Planet cloud ice kg", ledger.cloudIce);
    add("Planet falling kg", ledger.precipitation);
    add("Planet soil water kg", ledger.soil);
    add("Planet snow kg", ledger.snow);
    add("Planet runoff kg", ledger.runoff);
}

} // namespace

Inspection inspect(Game& game, double x, double z) {
    sim::World& world = game.nature.world;
    const sim::Vec2 place{x, z};
    const sim::SurfaceSample sample = sim::surfaceAt(world, place, sim::Level::Ground);
    Inspection record;
    record.emplace_back("level", std::string(LevelNames[sample.found ? size_t(sample.level) : 0]));
    if (!sample.found) return record;
    record.emplace_back("ground", double(sample.ground));
    record.emplace_back("rock", double(sample.rock));
    record.emplace_back("water", double(sample.water));
    record.emplace_back("coarse", double(sample.sediment[sim::Coarse]));
    record.emplace_back("sand", double(sample.sediment[sim::Sand]));
    record.emplace_back("fines", double(sample.sediment[sim::Fines]));
    record.emplace_back("discharge", double(sample.discharge));
    record.emplace_back("rain", double(sample.rain));
    record.emplace_back("warmth", double(sample.warmth));
    record.emplace_back("erosionRate", double(sample.erosionRate));
    record.emplace_back("depositionRate", double(sample.depositionRate));
    record.emplace_back("bedrock", std::string(materialInfo(sample.bedrock).name));
    if (surveyed(world, place))
        for (auto& [label, value] : sim::describe(sim::inspectAt(world, place))) record.emplace_back(std::move(label), std::move(value));
    describeWeather(record, game, place, sample.ground);
    return record;
}

} // namespace derivean
