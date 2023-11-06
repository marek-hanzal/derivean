import {
    includes,
    match,
    print,
    project,
    query
}              from "@phenomnomnominal/tsquery";
import {Timer} from "@use-pico/utils";
import fs      from "node:fs";
import {keyOf} from "../utils/keyOf";

export namespace withTx {
    export interface Props {
        packages: string[];
        output: string;
        locales: string[];
    }
}

export const withTx = (
    {
        packages,
        output,
        locales,
    }: withTx.Props,
) => {
    const translations: Record<string, { key: string, value: string }> = {};

    packages.forEach(path => {
        const benchmark = Timer.benchmark(() => {
            console.log(`Searching in [${path}/tsconfig.json]`);
            project(`${path}/tsconfig.json`)
                .filter(source => !source.fileName.endsWith(".d.ts"))
                .forEach(source => {
                    query(source, "TaggedTemplateExpression")
                        .filter(node => includes(node, "Identifier[name=tx]"))
                        .forEach(node => {
                            match(node, "NoSubstitutionTemplateLiteral").forEach(node => {
                                const source = print(node);
                                const text = source.substring(1, source.length - 1);
                                translations[keyOf(text)] = {
                                    key:   text,
                                    value: text,
                                };
                            });
                        });
                });
        });
        console.log(benchmark.format(`Package [${packages}] search time %s.%ms s`));
    });

    fs.mkdirSync(output, {recursive: true});

    const benchmark = Timer.benchmark(() => {
        locales.forEach(locale => {
            const target = `${output}/${locale}.json`;

            console.log(`Writing locale [${locale}] to [${target}]`);

            let current = {};
            try {
                current = JSON.parse(fs.readFileSync(target, {encoding: "utf-8"})) as Record<string, any>;
            } catch (e) {
            }

            fs.writeFileSync(target, JSON.stringify({
                ...translations,
                ...current
            }), {
                encoding: "utf-8",
            });
        });
    });

    console.log(benchmark.format("Exported in %s.%ms s"));
};
