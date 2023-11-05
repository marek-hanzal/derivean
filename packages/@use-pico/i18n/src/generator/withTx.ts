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

        mapper?(text: string): any;

        hash?(key: string): string;
    }
}

export const withTx = (
    {
        packages,
        output,
        locales,
        mapper = text => ({
            key:         text,
            translation: text,
        }),
        hash = keyOf,
    }: withTx.Props,
) => {
    console.log("Yep, i'm here");

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
                        translations[hash(text)] = mapper(text);
                    });
                });
        });
    });

    fs.mkdirSync(output, {recursive: true});

    locales.forEach(locale => {
        const target = `${output}/${locale}-t.json`;

        console.log(`Writing locale [${locale}] to [${target}]`);

        fs.writeFileSync(target, JSON.stringify(translations), {
            encoding: "utf-8",
        });
    });
};
