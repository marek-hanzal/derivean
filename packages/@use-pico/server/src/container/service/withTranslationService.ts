import {type ITranslationService} from "../../api/ITranslationService";
import {withService}              from "../../service/withService";

export const withTranslationService = withService<ITranslationService>("@use-pico/server/TranslationService");
