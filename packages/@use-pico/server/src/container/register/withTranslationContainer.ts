import {type IContainer}        from "../../api/IContainer";
import {TranslationService}     from "../../translator/service/TranslationService";
import {withTranslationService} from "../service/withTranslationService";

export const withTranslationContainer: IContainer.Register = container => {
    withTranslationService.bind(container, TranslationService);
};
