import {translation}         from "./translation";
import {TranslationInstance} from "./TranslationInstance";

/**
 * Simple text translation; supports (usually) only text interpolation, but cannot expand any components (like bold and so on).
 */
export function tx(input: TemplateStringsArray): string {
    return TranslationInstance.instance.pipeline.text.reduce(
        (text, current) => {
            return current({
                text,
                values: {},
            });
        },
        translation(input.join("")) as string
    );
}
