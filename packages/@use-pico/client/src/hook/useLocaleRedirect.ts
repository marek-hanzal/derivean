"use client";

import {useEffect}       from "react";
import {type IHrefProps} from "../api/IHrefProps";
import {useLocaleRouter} from "./useLocaleRouter";

/**
 * Instantly redirects to the specified url.
 */
export const useLocaleRedirect = (href: IHrefProps) => {
    const {push} = useLocaleRouter();
    useEffect(() => {
        push(href);
    }, []);
};
