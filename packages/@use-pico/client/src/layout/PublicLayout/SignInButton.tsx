"use client";

import {t}               from "@use-pico/translator";
import {
    signIn,
    type SignInOptions
}                        from "next-auth/react";
import {type FC}         from "react";
import {useLocaleRouter} from "../../hook/useLocaleRouter";
import {Button}          from "../../ui/Button";

export namespace SignInButton {
    export interface Props {
        loginUrl?: string;
        signInOptions?: SignInOptions;
    }
}

export const SignInButton: FC<SignInButton.Props> = (
    {
        loginUrl,
        signInOptions,
    }
) => {
    const router = useLocaleRouter();

    return <Button
        leftSection={<div className={"i-tabler-login"}/>}
        onClick={() => loginUrl ? router.push({
            href: loginUrl,
        }) : signIn(undefined, signInOptions)}
    >
        {t`Sign-in`}
    </Button>;
};
