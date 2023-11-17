"use client";

import {type FC}         from "react";
import {useDetectLocale} from "../hook/useDetectLocale";

export namespace DetectLocale {
    export type Props = useDetectLocale.Props;
}

/**
 * This is a utility component used to detect locale and call callback when it's detected.
 */
export const DetectLocale: FC<DetectLocale.Props> = (
    {
        locale,
        callback,
    }) => {
    useDetectLocale({
        locale,
        callback
    });
    return null;
};
