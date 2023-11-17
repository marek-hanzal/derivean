import {type TranslationSchema} from "@use-pico/translator";

export interface ITranslationService {
    translations(locale: string): Promise<Record<string, TranslationSchema.Type>>;
}
