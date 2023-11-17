import {type IContainer}        from "index";
import {TranslationService}     from "../../translator/service/TranslationService";
import {withTranslationService} from "../service/withTranslationService";

export const withTranslationContainer: IContainer.Register = container => {
    withTranslationService.bind(container, TranslationService);
};
