import {withTx} from "@use-pico/i18n-server";

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
