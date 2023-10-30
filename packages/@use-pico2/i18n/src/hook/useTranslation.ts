import {isString}              from "@use-pico2/utils";
import {WithTranslationStore}  from "../store/WithTranslationStore";
import {isTranslation}         from "../utils/isTranslation";
import {type IWithTranslation} from "../utils/IWithTranslation";

export type IUseTranslation = (label?: string, values?: Record<string, any>) => string;

export const useTranslation = (input?: string | IWithTranslation): IUseTranslation => {
    const withTranslation = WithTranslationStore.use$()?.withTranslation(isTranslation(input) ? input : undefined) || input;

    return (label) => {
        const value = isString(withTranslation) ? [withTranslation] : [
            withTranslation?.namespace,
            withTranslation?.label,
            withTranslation?.withLabel,
            label,
        ];
        return value.filter(Boolean).join(".");
    };
    // return useTranslations(
    //     isString(withTranslation) ?
    //         withTranslation :
    //         isTranslation(withTranslation) ?
    //             [
    //                 withTranslation.namespace,
    //                 withTranslation.label,
    //             ].filter(Boolean).join(".") :
    //             undefined
    // );
};