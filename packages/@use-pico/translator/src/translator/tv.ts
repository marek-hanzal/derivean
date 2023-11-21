import {type ReactNode}      from "react";
import {translation}         from "./translation";
import {TranslationInstance} from "./TranslationInstance";

/**
 * Default RichText translation function (returns ReactNode); if you need simple text translations, use `tx`.
 */
export function tv(
    values?: Record<string, any>,
    fallback?: ReactNode,
) {
    return (input: TemplateStringsArray): ReactNode => {
        return TranslationInstance.instance.pipeline.rich.reduce<ReactNode>(
            (text, current) => {
                return current({
                    text,
                    values,
                });
            },
            translation(input.join(""), fallback)
        );
    };
}