"use client";

import {type FC}                 from "react";
import {useAuthenticatedSession} from "../hook/useAuthenticatedSession";

export namespace CheckAuthenticatedSession {
    export interface Props {
        redirect?: string;
    }
}

export const CheckAuthenticatedSession: FC<CheckAuthenticatedSession.Props> = (
    {
        redirect,
    }
) => {
    useAuthenticatedSession({redirect});

    return null;
};
