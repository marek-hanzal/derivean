"use client";

import {useMemo}  from "react";
import {localeOf} from "../tools/localeOf";

export namespace useLocaleOf {
    export type Props = localeOf.Props;
}

/**
 * Simpler hook used only to detect user's locale.
 */
export const useLocaleOf = (props: useLocaleOf.Props) => {
    return useMemo(() => localeOf(props), [props.fallback, ...props.available]);
};
