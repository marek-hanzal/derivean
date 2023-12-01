"use client";

import {
    ActionIcon,
    useLocaleLinkTo
}                from "@use-pico/client";
import {signOut} from "next-auth/react";
import {type FC} from "react";

export namespace SignOutButton {
    export interface Props extends ActionIcon.Props {
    }
}

export const SignOutButton: FC<SignOutButton.Props> = props => {
    const linkTo = useLocaleLinkTo();

    return <ActionIcon
        size={"compact-lg"}
        onClick={() => signOut({
            callbackUrl: linkTo("/public"),
        })}
        {...props}
    >
        <div className={"i-tabler:logout"}/>
    </ActionIcon>;
};
