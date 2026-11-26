# Credits: reference data for comparison with Earth

Part of [Credits](../credits.md). These are measurements the generated planet and its models are checked against rather than models they implement. Earth is the reference: where the planet disagrees with these numbers, the planet is wrong. Measured values that live next to the model they check are credited there: Hulme's stopped lava flows under [volcanism](volcanism.md), JPL Horizons and the eclipse canons under [astronomy](astronomy-geometry.md).

## Earth as a whole (`docs/reference-landscapes.md:780`–`813`, compared in `docs/handoff/planet.md:14`)

### Cohen and Small 1998: hypsometry of ice-free land
Cohen, J. E., Small, C. (1998). Hypsographic demography: the distribution of human population by altitude. *Proceedings of the National Academy of Sciences* 95(24), 14009–14014. doi:10.1073/pnas.95.24.14009
- **Reference data:** land by height band, mean 673 m, quartiles 161, 408 and 846 m (30″ data).
- **Compared by:** `@simulation/tool/MeasureEarth.cpp:19`, `@simulation/tool/PlanetCommand.cpp:115` (the same bands).

### Dutch: hypsometry tables
Dutch, S. Global topographic summary. University of Wisconsin–Green Bay. https://stevedutch.net/usefuldata/earthhypso5min.htm (ETOPO5, 5′) and https://stevedutch.net/usefuldata/earthhypso1deg.htm (1°, after the RAND tables of Gates and Nelson 1973 and 1975)
- **Reference data:** land above 2 000 m by continent; the same bands with the ice sheets counted.

### Meybeck, Green and Vörösmarty 2001: relief classes
Meybeck, M., Green, P., Vörösmarty, C. (2001). A new typology for mountains and other relief classes: an application to global continental water resources and population distribution. *Mountain Research and Development* 21(1), 34–45. doi:10.1659/0276-4741(2001)021[0034:ANTFMA]2.0.CO;2
- **Reference data:** plains, lowlands, platforms and hills, plateaus and mountains by relief roughness; 13 % of non-glaciated land drains to no ocean.

### Mountains by other definitions
Sayre, R., Frye, C., Karagulle, D., et al. (2018). A new high-resolution map of world mountains and an online tool for visualizing and comparing characterizations of global mountain distributions. *Mountain Research and Development* 38(3), 240–249. doi:10.1659/MRD-JOURNAL-D-17-00107.1

Körner, C., Urbach, D., Paulsen, J. (2021). Mountain definitions and their consequences. *Alpine Botany* 131(2), 213–217. doi:10.1007/s00035-021-00265-8

Körner, C., Jetz, W., Paulsen, J., et al. (2017). A global inventory of mountains for bio-geographical applications. *Alpine Botany* 127(1), 1–15. doi:10.1007/s00035-016-0182-6

Kapos, V., Rhind, J., Edwards, M., et al. (2000). Developing a map of the world's mountain forests. In: *Forests in Sustainable Mountain Development*, 4–19. CABI/IUFRO. doi:10.1079/9780851994468.0004

Karagulle, D., Frye, C., Sayre, R., et al. (2017). Modeling global Hammond landform regions from 250-m elevation data. *Transactions in GIS* 21(5), 1040–1060. doi:10.1111/tgis.12265

Global Mountain Biodiversity Assessment (GMBA), mountain definitions: https://www.gmba.unibe.ch/services/tools/mountain_definition
- **Reference data:** 12.3 % (ruggedness, Körner), 24–26 % (Kapos), about 30 % (Hammond landforms, Karagulle) of land as mountains.

### Slopes of real mountains
Korup, O., Schmidt, J., McSaveney, M. J. (2005). Regional relief characteristics and denudation pattern of the western Southern Alps, New Zealand. *Geomorphology* 71(3–4), 402–423. doi:10.1016/j.geomorph.2005.04.013

DiBiase, R. A., Heimsath, A. M., Whipple, K. X. (2012). Hillslope response to tectonic forcing in threshold landscapes. *Earth Surface Processes and Landforms* 37(8), 855–865. doi:10.1002/esp.3205

Burbank, D. W., Leland, J., Fielding, E., et al. (1996). Bedrock incision, rock uplift and threshold hillslopes in the northwestern Himalayas. *Nature* 379(6565), 505–510. doi:10.1038/379505a0

Loye, A., Jaboyedoff, M., Pedrazzini, A. (2009). Identification of potential rockfall source areas at a regional scale using a DEM-based geomorphometric analysis. *Natural Hazards and Earth System Sciences* 9(5), 1643–1653. doi:10.5194/nhess-9-1643-2009
- **Reference data:** mean slopes near 30° at 25 m, threshold hillslopes of 35–40°, debris slopes peaking at 30–36° and cliffs from 46–54° at 1 m.

### Rock, ice and karst at the surface
Hartmann, J., Moosdorf, N. (2012). The new global lithological map database GLiM: a representation of rock properties at the Earth surface. *Geochemistry, Geophysics, Geosystems* 13(12). doi:10.1029/2012GC004370; dataset: doi:10.1594/PANGAEA.788537

Goldscheider, N., Chen, Z., Auler, A. S., et al. (2020). Global distribution of carbonate rocks and karst water resources. *Hydrogeology Journal* 28(5), 1661–1677. doi:10.1007/s10040-020-02139-5

National Snow and Ice Data Center. Glacier quick facts. https://nsidc.org/learn/parts-cryosphere/glaciers/glacier-quick-facts
- **Reference data:** sedimentary 64 %, metamorphic 13 %, plutonic 7 %, volcanic 6 % of land; karst on 15.2 % of ice-free land; ice on 10 % of land today and 32 % at the last glacial maximum.

### Drainage density
U.S. Geological Survey, effect of climate on drainage density and streamflow: https://www.usgs.gov/publications/effect-climate-drainage-density-and-streamflow; Schumm's classes as summarised at https://taylorandfrancis.com/knowledge/Engineering_and_technology/Engineering_support_and_special_topics/Drainage_density (unverified: the underlying USGS paper's authors and year)

## Measured lakes (the evaporation patch, `docs/handoff/groundwater.md:49`, `:50`)

The checks are held with the evaporation patch in `.out/groundwater/evap/patched/@simulation/test/Evaporation.cpp:19`–`21`.

### Elsawwaf et al. 2010: Lake Nasser
Elsawwaf, M., Willems, P., Pagano, A., Berlamont, J. (2010). Evaporation estimates from Nasser Lake, Egypt, based on three floating station data and Bowen ratio energy budget. *Theoretical and Applied Climatology* 100(3–4), 439–465. doi:10.1007/s00704-009-0168-z
- **Reference data:** about 2.2 m/yr; also calibrates the advected share of the evaporation.

### Moreo and Swancar 2013: Lake Mead
Moreo, M. T., Swancar, A. (2013). *Evaporation from Lake Mead, Nevada and Arizona, March 2010 through February 2012*. U.S. Geological Survey Scientific Investigations Report 2013-5229. doi:10.3133/sir20135229
- **Reference data:** 1.88–2.07 m/yr by eddy covariance.

### Yin and Nicholson 1998: Lake Victoria
Yin, X., Nicholson, S. E. (1998). The water balance of Lake Victoria. *Hydrological Sciences Journal* 43(5), 789–811. doi:10.1080/02626669809492173
- **Reference data:** 1.55 m/yr evaporation for 1956–78 against 1.79 m of rain on the lake.

### Delclaux, Coudrain and Condom 2007; Pillco Zolá et al. 2019: Lake Titicaca
Delclaux, F., Coudrain, A., Condom, T. (2007). Evaporation estimation on Lake Titicaca: a synthesis review and modelling. *Hydrological Processes* 21(13), 1664–1677. doi:10.1002/hyp.6360

Pillco Zolá, R., Bengtsson, L., Berndtsson, R., et al. (2019). Modelling Lake Titicaca's daily and monthly evaporation. *Hydrology and Earth System Sciences* 23(2), 657–668. doi:10.5194/hess-23-657-2019
- **Reference data:** about 1.65 m/yr, 1.70 as an upper limit, at 3 812 m.

### Lake Geneva and Great Salt Lake
Hydrological Atlas of Switzerland (Geneva, 0.94–0.95 m/yr by mass balance) and USGS (Great Salt Lake, 1.0–1.15 m/yr), named in `Evaporation.cpp:20`–`21` (held) without a specific plate or report ([not yet identified](../credits.md#not-yet-identified)).

## Landscape catalogue

`docs/reference-landscapes.md` links each landscape type to photographs on Wikimedia Commons and takes many dimensions from Wikipedia articles (Yosemite Valley, Lauterbrunnen Valley, Grand Canyon, fjords, drumlins, Skeiðarársandur and others). Those pages are linked where they are used and are not repeated here; figures marked † there were quoted from the literature without re-checking.
