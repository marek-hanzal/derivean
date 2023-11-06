import {
    includes,
    match,
    print,
    project,
    query
}              from "@phenomnomnominal/tsquery";
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
    const translations: Record<string, any> = {};

    packages.forEach(path => {
        console.log(`Searching in [${path}/tsconfig.json]`);
        project(`${path}/tsconfig.json`).forEach(source => {
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

    fs.mkdirSync(output, {recursive: true});

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
};
