import {TranslationInstance} from "../instance/TranslationInstance";
import {keyOf}               from "../utils/keyOf";

export namespace tx {
    export interface Props {
        values?: Record<string, any>;
        fallback?: string;
    }
}

export const tx = (
    props?: tx.Props
) => {
    return (input: TemplateStringsArray): string | undefined => {
        const key = input.join("");
        return TranslationInstance.instance.translations[keyOf(key)]?.["value"] ?? props?.fallback ?? key;
    };
};
