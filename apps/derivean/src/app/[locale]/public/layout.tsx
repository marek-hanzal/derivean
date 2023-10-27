import {PublicLayout}           from "@derivean/public";
import {SignInButton}           from "@use-pico/ui";
import {type PropsWithChildren} from "react";
import logo                     from "../../../../public/assets/logo/logo.svg";

export namespace Layout {
    export type Props = PropsWithChildren;
}

export default function Layout(
    {
        children,
    }: Layout.Props
) {
    return <PublicLayout
        logo={logo}
        withoutLogin={true}
        right={<SignInButton/>}
    >
        {children}
    </PublicLayout>;
}
