import {withRpc}           from "@use-pico/extras";
import {TranslationSchema} from "@use-pico/server";

export const TranslationRpc = withRpc({
    key:    ["pico", "translation"],
    schema: TranslationSchema,
});
export type TranslationRpc = typeof TranslationRpc;
