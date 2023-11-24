import {translation}         from "./translation";
import {TranslationInstance} from "./TranslationInstance";

/**
 * Simple text dynamic translations (excluded from automatic translation extractor)
 */
export function td(input: string): string {
    return TranslationInstance.instance.pipeline.text.reduce(
        (text, current) => {
            return current({
                text,
                values: {},
            });
        },
        translation(input) as string
    );
}
