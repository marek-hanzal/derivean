# Zadání vlastníka: oblaka

Původní znění, jak ho vlastník vložil během práce.

---

# DeRivean – Atmospheric cloud simulation and volumetric rendering (owner's brief, verbatim)

Implement a comprehensive, physically inspired cloud system as a first-class citizen of the existing world simulation.

The goal is not merely to render visually appealing clouds. Clouds must represent the actual atmospheric state, respond to environmental conditions, participate in the water cycle, and produce recognizable, naturally evolving cloud formations.

## 1. Atmospheric integration
Cloud formation must be driven by the atmospheric simulation, including: atmospheric temperature and its vertical profile; atmospheric pressure; water vapor content and relative humidity; condensation and evaporation; atmospheric stability and instability; vertical convection and uplift; horizontal wind and vertical wind shear; orographic uplift caused by terrain; seasonal and diurnal temperature changes; available moisture originating from oceans, lakes, rivers and wet ground.
Use physically meaningful saturation and condensation relationships, such as the Clausius–Clapeyron relation.
Distinguish between atmospheric water vapor, condensed liquid water, ice crystals and precipitation.
Clouds must participate in the water cycle. Atmospheric transport, condensation, evaporation and precipitation must respect water mass conservation, including exchanges across simulation region boundaries.
Avoid introducing a separate, disconnected weather system solely for rendering clouds.

## 2. Cloud classification and morphology
Support all ten cloud genera recognized by the WMO:
- Cumulus — individual convective clouds with relatively flat bases and rounded, vertically developing tops
- Cumulonimbus — massive vertically developed thunderstorm clouds, potentially with anvil-shaped tops
- Stratus — low, relatively uniform sheets of cloud
- Stratocumulus — extensive low-level layers with irregular cellular and rounded structures
- Nimbostratus — thick, extensive precipitation-producing cloud layers
- Altocumulus — mid-level groups of rounded cloud masses and patches
- Altostratus — extensive mid-level cloud sheets
- Cirrus — high-altitude, thin, fibrous ice clouds shaped by wind
- Cirrocumulus — high-altitude fields of small, rippled cloud formations
- Cirrostratus — thin, extensive, translucent high-altitude cloud layers
Reference: WMO International Cloud Atlas https://cloudatlas.wmo.int/en/home.html
Cloud genera should emerge from, or be derived from, atmospheric conditions rather than arbitrary random selection.
Respect the characteristic: cloud base and top altitudes; vertical development; horizontal extent; density and internal structure; atmospheric stability; liquid water and ice content; precipitation potential; wind-driven deformation.
Cloud altitudes must not be universally fixed. They should respond to atmospheric temperature, pressure and climatic conditions.
Support continuous transitions between cloud formations. A developing cumulus should be capable of growing into a cumulonimbus under suitable conditions.
Cloud genera must not be implemented as ten static meshes or ten unrelated visual presets.

## 3. Volumetric representation and rendering
Implement genuinely three-dimensional volumetric clouds.
Prefer GPU ray marching through a continuous three-dimensional cloud density field rather than flat sprites, billboards, sky textures or opaque voxel boxes.
The cloud representation should support: continuous volumetric density; naturally irregular silhouettes; fine-scale turbulent structures; layered and vertically developed formations; variable optical thickness; gradual density transitions; wind-driven movement and deformation; simultaneous cloud formations at different altitudes.
Procedural noise is acceptable for reconstructing unresolved small-scale atmospheric structures, provided that the overall cloud geometry, altitude, density and distribution remain constrained by the simulated atmospheric state.
Implement physically inspired optical effects: Henyey–Greenstein phase function for directional light scattering; Beer–Lambert attenuation; volumetric self-shadowing; direct sunlight and ambient atmospheric illumination; appropriate translucency and optical differences between liquid-water and ice clouds; cloud illumination responding to sun position, including sunrise and sunset.
The final visual result must resemble recognizable natural clouds rather than generic volumetric blobs.
Clouds must remain spatially coherent when viewed from below, above, from the side and from within the cloud volume.

## 4. Cloud shadows and environmental interaction
Clouds must cast spatially corresponding shadows onto the terrain. Shadow movement must follow actual cloud movement and sun position.
Where supported by the existing physical model, cloud coverage and optical thickness should also influence surface solar heating and therefore local temperature and evaporation.
Ensure consistency between: visible cloud coverage; ground illumination; atmospheric moisture; precipitation; surface water accumulation; snow accumulation and melting; hydrological runoff.
Precipitation must originate from simulated atmospheric conditions, not from an independent randomized weather timer.
Clouds should naturally form, evolve, merge, dissipate and precipitate as atmospheric conditions change.

## 5. Terrain and atmospheric feedback
Support atmospheric phenomena resulting from interactions with the landscape: orographic cloud formation above mountains; windward precipitation; leeward rain shadows; low-level fog and cloud formation in valleys; moisture transport from oceans and lakes; convective cloud development above heated terrain; seasonal changes in cloud distribution, altitude and precipitation type.
Cloud systems must not be independent of geography.

## 6. Infinite-world compatibility
DeRivean represents a potentially infinite planar world rather than a spherical planet. The cloud system must therefore support: deterministic, spatially coherent atmospheric fields; continuous cloud formations across chunk and simulation-region boundaries; persistent world-space cloud positions; consistent evolution independent of player movement and chunk loading order; atmospheric transport between neighboring regions; multiple spatial and temporal simulation resolutions.
Avoid visible seams, repeating cloud patterns, camera-relative cloud movement and sudden changes when crossing region boundaries.
Handle floating-point precision, camera far-plane limitations and sky-volume clipping correctly.
The rendering proxy must not impose artificial limits on physically meaningful cloud altitudes or the atmospheric simulation.

## 7. Performance and architecture
Separate atmospheric simulation, volumetric density reconstruction and rendering.
The renderer must consume the simulated atmospheric state without becoming the authoritative source of weather data.
Use appropriate performance techniques where necessary: adaptive ray-marching step sizes; early ray termination; reduced-resolution volumetric rendering; temporal reprojection; spatial and temporal LOD; GPU-side density reconstruction; efficient cloud-shadow sampling.
Do not unnecessarily simulate atmospheric microphysics at the resolution of terrain voxels.
Prefer meaningful physical approximations over computationally impractical literal simulations.

## 8. Simulation inspector
Extend the existing Simulation Inspector with relevant atmospheric diagnostics: water vapor and relative humidity; condensed cloud water and ice; atmospheric stability; horizontal and vertical airflow; cloud coverage; cloud base and top altitude; cloud genus classification; precipitation intensity and type; cloud optical thickness; atmospheric temperature profile; water mass balance.
The inspector should make it possible to explain why a particular cloud formation exists at a particular location and time.

## 9. Verification and acceptance criteria
Provide visual and automated verification of the implementation. Verify at minimum: all ten WMO cloud genera are distinguishable; clouds respond to changing atmospheric conditions; different cloud formations can coexist; cloud movement follows atmospheric transport; mountains influence cloud formation and precipitation; clouds cast consistent terrain shadows; atmospheric water is conserved within the defined model; cloud formations remain continuous across region boundaries; results are independent of chunk loading order; no visible clipping, artificial sky boundaries or major rendering artifacts occur; performance remains suitable for real-time exploration.
Capture representative screenshots from actual running scenes, including clear weather, layered overcast, convective clouds, thunderstorms, high-altitude ice clouds and mountainous terrain.
Use controlled atmospheric test scenarios to verify individual cloud genera, but do not introduce artificial cloud spawning into the production simulation merely to satisfy visual tests.
Run the complete workspace build and test suite, and perform an independent code and architecture review.
Explicitly document which processes are physically simulated, analytically approximated, procedurally reconstructed or purely visual.

## Core principle
Do not generate clouds independently of the atmosphere. Simulate the atmospheric conditions from which clouds arise, and render their resulting three-dimensional structure. Weather is part of the world, not an effect applied on top of it.
