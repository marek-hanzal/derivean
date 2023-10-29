import {type WithQuery}         from "@use-pico2/query";
import {type TranslationSchema} from "../schema/TranslationSchema";
import {type WithLocaleSchema}  from "../schema/WithLocaleSchema";

export type IWithTranslationQuery = WithQuery<WithLocaleSchema, TranslationSchema>;
