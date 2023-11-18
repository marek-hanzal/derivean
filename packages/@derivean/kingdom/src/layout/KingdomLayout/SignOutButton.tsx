"use client";

import {IconLogout} from "@tabler/icons-react";
import {
    ActionIcon,
    useLocaleLinkTo
}                   from "@use-pico/client";
import {signOut}    from "next-auth/react";
import {type FC}    from "react";

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
