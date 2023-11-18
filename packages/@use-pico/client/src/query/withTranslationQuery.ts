import {WithLocaleSchema}   from "@use-pico/server";
import {TranslationsSchema} from "@use-pico/translator";
import {withRpcQuery}       from "../rpc/withRpcQuery";

export const withTranslationQuery = withRpcQuery({
    key:    ["pico", "i18n", "translation", "query"],
    schema: {
        request:  WithLocaleSchema,
        response: TranslationsSchema,
    },
});
