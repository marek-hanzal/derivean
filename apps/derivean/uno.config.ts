import {
    defineConfig,
    presetIcons,
    presetUno,
    presetWind,
    transformerVariantGroup
} from "unocss";

export default defineConfig({
    presets:      [
        presetUno(),
        presetWind(),
        presetIcons(),
    ],
    transformers: [
        transformerVariantGroup(),
    ],
});
