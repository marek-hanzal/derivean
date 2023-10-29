"use client";

import {IconLogout}      from "@tabler/icons-react";
import {useLocaleLinkTo} from "@use-pico2/i18n";
import {ActionIcon}      from "@use-pico2/ui";
import {signOut}         from "next-auth/react";
import {type FC}         from "react";

export namespace SignOutButton {
    export interface Props extends ActionIcon.Props {
    }
}

export const SignOutButton: FC<SignOutButton.Props> = props => {
    const linkTo = useLocaleLinkTo();

    return <ActionIcon
        variant={"subtle"}
        size={"md"}
        onClick={() => signOut({
            callbackUrl: linkTo("/public"),
        })}
        {...props}
    >
        <IconLogout/>
    </ActionIcon>;
};
