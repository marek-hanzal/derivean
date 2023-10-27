"use client";

import {IconLogin}   from "@tabler/icons-react";
import {Translation} from "@use-pico/i18n";
import {Button}      from "@use-pico/ui";
import {signIn}      from "next-auth/react";
import {type FC}     from "react";

export namespace SignInButton {
    export type Props = Button.Props;
}

export const SignInButton: FC<SignInButton.Props> = props => {
    return <Button
        leftSection={<IconLogin/>}
        size={"md"}
        onClick={() => signIn()}
        {...props}
    >
        <Translation
            namespace={"public"}
            withLabel={"sign-in.button"}
        />
    </Button>;
};
