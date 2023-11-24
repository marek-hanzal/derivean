import {translation}         from "./translation";
import {TranslationInstance} from "./TranslationInstance";

export namespace tdv {
    export interface Props {
        values?: Record<string, any>;
        fallback?: string;
    }
}

/**
 * Simple text dynamic translations (excluded from automatic translation extractor)
 */
export function tdv(
    props?: tdv.Props
) {
    return (input: string): string => {
        return TranslationInstance.instance.pipeline.text.reduce(
            (text, current) => {
                return current({
                    text,
                    values: props?.values,
                });
            },
            translation(input, props?.fallback) as string
        );
    };
}
