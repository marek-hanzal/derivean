import {type IContainer}        from "@use-pico/server";
import {TranslationService}     from "../translator/service/TranslationService";
import {withTranslationService} from "./withTranslationService";

export const withTranslationContainer: IContainer.Register = container => {
    withTranslationService.bind(container, TranslationService);
};
