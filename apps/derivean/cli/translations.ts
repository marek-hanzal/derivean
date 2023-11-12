import {withTx} from "@use-pico/tx";

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
