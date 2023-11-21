import {withTx} from "@use-pico/server/src/generator/withTx";

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
