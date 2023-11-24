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
        pipeline: {
            include: [
                /\.(tsx?)($|\?)/,
            ],
        },
    },
    presets:      [
        presetUno(),
        presetWebFonts({
            provider: "google",
            fonts:    {
                sans: "Roboto",
                mono: ["Fire Code", "Fira Mono:400,700"],
            }
        }),
        presetIcons(),
    ],
    transformers: [
        transformerVariantGroup(),
        transformerCompileClass(),
    ],
});
