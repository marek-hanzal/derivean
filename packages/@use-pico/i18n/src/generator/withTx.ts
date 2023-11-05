import {
    ast,
    includes,
    match,
    print,
    query
} from "@phenomnomnominal/tsquery";

export namespace withTx {
    export interface Props {
    }
}

export const withTx = () => {
    console.log("Yep, i'm here");

    const parsed = ast("const a = true; tx({foo: 'bar'})`Some translation`  anotherTagger`blablabla`");
    const nodes = query(parsed, "TaggedTemplateExpression");

    nodes.forEach(node => {
        if (includes(node, "Identifier[name=tx]")) {
            match(node, "NoSubstitutionTemplateLiteral").forEach(node => {
                console.log("Translation", print(node));
            });
        }
    });
};
