"use client";

import {type FC}      from "react";
import {useOnSession} from "../hook/useOnSession";

export namespace OnSession {
    export interface Props extends useOnSession.Props {
    }
}
/**
 * Just a wrapper component around useOnSession hook.
 */
export const OnSession: FC<OnSession.Props> = props => {
    useOnSession(props);

    return null;
};
