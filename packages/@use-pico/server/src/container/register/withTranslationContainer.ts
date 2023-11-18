import {
    TranslationRpc,
    withTranslationQuery
}                                  from "@use-pico/client";
import {type IContainer}           from "../../api/IContainer";
import {TranslationRepository}     from "../../translator/repository/TranslationRepository";
import {TranslationService}        from "../../translator/service/TranslationService";
import {withHandler}               from "../../utils/withHandler";
import {withRepositoryHandler}     from "../../utils/withRepositoryHandler";
import {withTranslationRepository} from "../service/withTranslationRepository";
import {withTranslationService}    from "../service/withTranslationService";

export const withTranslationContainer: IContainer.Register = container => {
    withTranslationService.bind(container, TranslationService);

    withRepositoryHandler({
        container,
        repository:     TranslationRepository,
        withRepository: withTranslationRepository,
        handler:        TranslationRpc,
    });

    withHandler({
        container,
        key:    withTranslationQuery.key,
        schema: withTranslationQuery.schema,
        async handle(
            {
                container,
                request
            }) {
            return {
                translations: await withTranslationService.use(container).translations(request.locale),
            };
        }
    });
};
