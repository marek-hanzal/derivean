import {withTx} from "@use-pico/i18n";

withTx({
    packages: [
        ".",
    ],
    output:   "./src/translation",
    locales:  [
        "cs",
        "en",
    ],
});
