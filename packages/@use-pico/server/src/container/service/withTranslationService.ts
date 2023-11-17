import {withService}              from "index";
import {type ITranslationService} from "../../api/ITranslationService";

export const withTranslationService = withService<ITranslationService>("@use-pico/server/TranslationService");
