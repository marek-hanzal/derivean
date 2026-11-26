# Credits: rendering

Part of [Credits](../credits.md); the entry format is explained there. The renderer draws what the simulation decides; the clean-air and haze optics it reads come from the [atmosphere](atmosphere.md).

### Bird and Hulstrom 1981: clear-sky light on the ground
Bird, R. E., Hulstrom, R. L. (1981). *A simplified clear sky model for direct and diffuse insolation on horizontal surfaces*. Technical Report SERI/TR-642-761, Solar Energy Research Institute, Golden, CO. doi:10.2172/6510849
- **Model:** the share of light scattered out of the sun's beam that reaches the ground (0.79), the forward share of haze scattering (0.84), and the split of the haze's transmittance into absorbed and scattered parts.
- **Used in:** `@game/src/sky/fn/clearSky.ts:6`, `:9`, `:42`, `:64`; `docs/architecture.md:85`; `docs/handoff/renderer.md:31`.
- **Approximations:** below a 1° sun the sky light follows Earth's civil twilight instead of the model, which would leave the ground black (`clearSky.ts:9`).

### Kasten and Young 1989: air mass
Kasten, F., Young, A. T. (1989). Revised optical air mass tables and approximation formula. *Applied Optics* 28(22), 4735. doi:10.1364/AO.28.004735
- **Model:** the air mass the sun's light crosses at a given height over the horizon.
- **Used in:** `@game/src/sky/fn/clearSky.ts:22`; `docs/handoff/renderer.md:31`.

### Hapke 1981: diffusive reflectance
Hapke, B. (1981). Bidirectional reflectance spectroscopy: 1. Theory. *Journal of Geophysical Research* 86(B4), 3039–3054. doi:10.1029/JB086iB04p03039
- **Model:** (1 − γ)/(1 + γ) with γ = √(1 − albedo), the light a surface returns to the sky.
- **Used in:** `@game/src/sky/fn/skyLight.ts:78`.

### Koschmieder 1924: how far one sees through the air
Koschmieder, H. (1924). Theorie der horizontalen Sichtweite. *Beiträge zur Physik der freien Atmosphäre* 12, 33–53.
- **Model:** a dark target fades to 2 % contrast after 3.912 / extinction; the view distance drawn through the haze.
- **Used in:** `@game/src/sky/fn/skyLight.ts:55` (cited by name only); `docs/handoff/atmosphere.md:69`.

### Henyey and Greenstein 1941: the phase function of cloud
Henyey, L. G., Greenstein, J. L. (1941). Diffuse radiation in the Galaxy. *The Astrophysical Journal* 93, 70. doi:10.1086/144246
- **Model:** forward-peaked scattering of sunlight inside cloud.
- **Used in:** `@game/src/clouds/fn/marchNode.ts:88` (cited by name only); `docs/handoff/brief/clouds.md:44`.

### Beer–Lambert attenuation
The exponential loss of the direct beam with optical depth (Bouguer 1729, Lambert 1760, Beer 1852), textbook physics without a single modern source; named in `@game/src/clouds/fn/sunThrough.ts:4` and `docs/handoff/brief/clouds.md:44`.

### Schlick 1994: Fresnel reflectance of water
Schlick, C. (1994). An inexpensive BRDF model for physically-based rendering. *Computer Graphics Forum* 13(3), 233–246. doi:10.1111/1467-8659.1330233
- **Model:** a mirror of the sky at grazing angles, clear when looked into.
- **Used in:** `@game/src/water/fn/sheetSurface.ts:36` (cited by name only).

### Perlin 1985, 2002; Worley 1996: noise for cloud structure
Perlin, K. (1985). An image synthesizer. *ACM SIGGRAPH Computer Graphics* 19(3), 287–296. doi:10.1145/325165.325247

Perlin, K. (2002). Improving noise. *ACM Transactions on Graphics* 21(3), 681–682. doi:10.1145/566654.566636

Worley, S. (1996). A cellular texture basis function. *Proceedings of SIGGRAPH 96*, ACM, 291–294. doi:10.1145/237170.237267
- **Model:** tileable gradient and cellular noise for the structure below a weather cell (heaped domes, smooth and cellular channels, fine wisps).
- **Used in:** `@game/src/clouds/constant/noise.ts:60`, `:61`, `:71` (cited by name only; which Perlin variant is not stated).
- **Used in (Perlin 2002, S3b):** 3D gradient noise with the quintic fade and twelve edge gradients for the planet on the sphere (plate warp, swell, primordial plumes), `@simulation/src/core/fn/Noise.hpp:16`, `@simulation/src/core/fn/Noise.cpp:83`.
- **Note:** the noise shapes cloud detail inside a simulated weather cell; where and how much cloud there is comes from the simulation (`docs/handoff/brief/clouds.md`).
