import {
    defineConfig,
    transformerVariantGroup
} from "unocss";

export default defineConfig({
    presets:      [
        // presetUno(),
        // presetWind(),
        // presetIcons(),
    ],
    transformers: [
        transformerVariantGroup(),
    ],
});
