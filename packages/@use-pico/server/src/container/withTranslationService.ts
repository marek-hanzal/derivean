import {withService}              from "@use-pico/server";
import {type ITranslationService} from "../api/ITranslationService";

export const withTranslationService = withService<ITranslationService>("@use-pico/translator/TranslationService");
