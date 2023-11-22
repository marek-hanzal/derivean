import {
    defineConfig,
    presetIcons,
    presetUno,
    presetWind
} from "unocss";

export default defineConfig({
    presets: [
        presetUno(),
        presetWind(),
        presetIcons(),
    ],
});
