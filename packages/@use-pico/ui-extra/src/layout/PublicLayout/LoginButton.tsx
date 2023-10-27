"use client";

import {IconLogin} from "@tabler/icons-react";
import {
    Translation,
    useLocaleRouter
}                  from "@use-pico/i18n";
import {Button}    from "@use-pico/ui";
import {signIn}    from "next-auth/react";
import {type FC}   from "react";

export namespace LoginButton {
    export interface Props {
        loginUrl?: string;
    }
}

export const LoginButton: FC<LoginButton.Props> = (
    {
        loginUrl,
    }
) => {
    const router = useLocaleRouter();

    return <Button
        leftSection={<IconLogin/>}
        onClick={() => loginUrl ? router.push({
            href: loginUrl,
        }) : signIn()}
    >
        <Translation
            namespace={"public"}
            withLabel={"button.sign-in"}
        />
    </Button>;
};
