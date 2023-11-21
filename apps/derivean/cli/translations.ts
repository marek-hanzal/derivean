import {withTx} from "@use-pico/server/generator/withTx";

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
