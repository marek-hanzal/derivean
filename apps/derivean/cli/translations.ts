import {withTx} from "@use-pico/i18n";

withTx({
    packages: [
        "./apps/derivean",
    ],
    output: "./src/translation",
    locales:  [
        "cs",
        "en",
    ],
});
