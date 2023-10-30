"use client";

import {type IHrefProps} from "@use-pico2/navigation";
import {useEffect}       from "react";
import {useLocaleRouter} from "./useLocaleRouter";

export const useLocaleRedirect = (href: IHrefProps) => {
    const {push} = useLocaleRouter();
    useEffect(() => {
        push(href);
    }, []);
};