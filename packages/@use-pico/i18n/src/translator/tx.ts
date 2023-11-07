import {TranslationInstance} from "../instance/TranslationInstance";
import {keyOf}               from "../utils/keyOf";

export namespace tx {
    export interface Props {
        values?: Record<string, any>;
        fallback?: string;
    }
}

export function tx(
    props?: tx.Props
) {
    return (input: TemplateStringsArray): string => {
        const key = input.join("");
        return TranslationInstance.instance.pipeline.reduce(
            (text, current) => {
                return current({
                    values: props?.values,
                    text,
                });
            },
            TranslationInstance.instance.translations[keyOf(key)]?.["value"] ?? props?.fallback ?? key
        );
    };
}
