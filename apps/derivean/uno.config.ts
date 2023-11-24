import {
    defineConfig,
    presetIcons,
    presetUno,
    transformerVariantGroup
} from "unocss";

export default defineConfig({
    content: {
        pipeline: {
            include: [
                "*.{js,ts,jsx,tsx}",
            ],
        },
    },
    presets:      [
        presetUno(),
        presetIcons(),
    ],
    transformers: [
        transformerVariantGroup(),
    ],
});
