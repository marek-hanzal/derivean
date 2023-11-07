import {TranslationInstance} from "../instance/TranslationInstance";
import {interpolate}         from "../utils/interpolate";
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
    return (input: TemplateStringsArray): string => {
        const key = input.join("");
        return interpolate(
            TranslationInstance.instance.translations[keyOf(key)]?.["value"] ?? props?.fallback ?? key,
            props?.values
        );
    };
};
