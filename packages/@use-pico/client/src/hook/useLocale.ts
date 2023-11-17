import {TranslationInstance} from "@use-pico/translator";

export const useLocale = () => {
    return TranslationInstance.instance.locale;
};
