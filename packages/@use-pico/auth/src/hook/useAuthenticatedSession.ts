"use client";

import {useWithRedirect} from "@use-pico/navigation";
import {useSession}      from "next-auth/react";
import {useEffect}       from "react";

export namespace useAuthenticatedSession {
    export interface Props {
        redirect?: string;
    }
}

export const useAuthenticatedSession = (
    {
        redirect,
    }: useAuthenticatedSession.Props
) => {
    const session = useSession();
    const redirect$ = useWithRedirect();
    useEffect(() => {
        (async () => {
            if (redirect && session.status === "authenticated") {
                redirect$(redirect);
            }
        })();
    }, [redirect, session.status]);
    return session;
};
