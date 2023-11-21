import {translation}         from "./translation";
import {TranslationInstance} from "./TranslationInstance";

/**
 * Simple text translation; supports (usually) only text interpolation, but cannot expand any components (like bold and so on).
 */
export function txv(
    values?: Record<string, any>,
    fallback?: string,
) {
    return (input: TemplateStringsArray): string => {
        return TranslationInstance.instance.pipeline.text.reduce(
            (text, current) => {
                return current({
                    text,
                    values,
                });
            },
            translation(input.join(""), fallback) as string
        );
    };
}
