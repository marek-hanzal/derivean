"use client";

import {
    type INavigate,
    useWithRedirect
}                  from "@use-pico/navigation";
import {
    type SessionContextValue,
    useSession
}                  from "next-auth/react";
import {useEffect} from "react";

export namespace useOnSession {
    export interface Props {
        /**
         * Called when there is an existing session
         */
        onAuth(navigate: INavigate, session: SessionContextValue<true>): void;

        /**
         * Called when there is not existing session
         */
        onUnknown(navigate: INavigate): void;
    }
}

export const useOnSession = (
    {
        onAuth,
        onUnknown,
    }: useOnSession.Props
) => {
    const session = useSession();
    const redirect = useWithRedirect();
    useEffect(() => {
        if (session.status === "authenticated") {
            onAuth(redirect, session);
        } else if (session.status === "unauthenticated") {
            onUnknown(redirect);
        }
    }, [session.status]);
    return session;
};
