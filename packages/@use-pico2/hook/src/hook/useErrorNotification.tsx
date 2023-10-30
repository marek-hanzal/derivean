"use client";

import {WithTranslationStore} from "@use-pico2/i18n";
import {useStore$}            from "@use-pico2/store";
import {
    AlertIcon,
    withNotification
}                             from "@use-pico2/ui";

export namespace useErrorNotification {
    export interface Props extends Omit<withNotification.Props, "title" | "message"> {
    }
}

export const useErrorNotification = () => {
    const translation = useStore$(WithTranslationStore);
    return (
        {
            withTranslation,
            ...props
        }: useErrorNotification.Props = {}
    ) => {
        const $withTranslation = {
            ...withTranslation,
            label: [withTranslation?.label, "error"].filter(Boolean).join("."),
        };
        withNotification({
            withTranslation: translation?.withTranslation($withTranslation) || $withTranslation,
            icon:            <AlertIcon color={"black"} size={"1.1rem"}/>,
            color:           "red",
            ...props,
        });
    };
};
