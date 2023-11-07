import {type IPipeline}     from "./IPipeline";
import {type ITranslations} from "./ITranslations";

export interface ITranslationInstance {
    locale: string;
    translations: ITranslations;
    pipeline: IPipeline[];
}
