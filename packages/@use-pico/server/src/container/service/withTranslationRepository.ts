import {withService}                from "../../service/withService";
import {type TranslationRepository} from "../../translator/repository/TranslationRepository";

export const withTranslationRepository = withService<TranslationRepository.Type>("@use-pico/server/TranslationRepository");
