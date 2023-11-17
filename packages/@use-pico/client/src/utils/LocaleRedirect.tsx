"use client";

import {useRouter}    from "next/navigation";
import {type FC}      from "react";
import {localeOf}     from "../tools/localeOf";
import {DetectLocale} from "./DetectLocale";

export namespace LocaleRedirect {
    export interface Props {
        target?: string;
        locale: localeOf.Props;
        detectLocaleProps?: Omit<DetectLocale.Props, "locale" | "callback">;
    }
}

/**
 * This is a utility component used to detect locale and call callback when it's detected.
 *
 * Client-side only.
 */
export const LocaleRedirect: FC<LocaleRedirect.Props> = (
    {
        target,
        locale,
        detectLocaleProps,
    }) => {
    const router = useRouter();

    return <DetectLocale
        locale={locale}
        callback={({locale}) => router.push(target ? `/${locale}/${target}` : `/${locale}`)}
        {...detectLocaleProps}
    />;
};
