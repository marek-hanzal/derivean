import {
    includes,
    match,
    print,
    project,
    query
} from "@phenomnomnominal/tsquery";

export namespace withTx {
    export interface Props {
        packages: string[];
    }
}

export const withTx = (
    {
        packages,
    }: withTx.Props,
) => {
    console.log("Yep, i'm here");

    packages.forEach(path => {
        console.log(`Searching in [${path}/tsconfig.json]`);
        project(`${path}/tsconfig.json`).forEach(source => {
            query(source, "TaggedTemplateExpression")
                .filter(node => includes(node, "Identifier[name=tx]"))
                .forEach(node => {
                    match(node, "NoSubstitutionTemplateLiteral").forEach(node => {
                        console.log("Found", print(node));
                    });
                });
        });
    });
};
