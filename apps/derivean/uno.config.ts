import {
    defineConfig,
    presetIcons,
    presetUno,
    presetWebFonts,
    transformerCompileClass,
    transformerVariantGroup
} from "unocss";

export default defineConfig({
    content:      {
        filesystem: [
            "src/**/*.{ts,tsx}",
            "../../packages/**/*.{ts,tsx}",
        ],
    },
    presets:      [
        presetUno(),
        presetWebFonts({
            provider: "google",
            fonts:    {
                sans: "Roboto",
                mono: ["Fire Code", "Fira Mono:400,700"],
            },
        }),
        presetIcons(),
    ],
    transformers: [
        transformerVariantGroup(),
        transformerCompileClass(),
    ],
});
