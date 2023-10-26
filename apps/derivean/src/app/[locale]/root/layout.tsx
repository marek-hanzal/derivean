import logo                     from "@/derivean/assets/logo/logo.svg";
import {RootLayout}             from "@derivean/root";
import {type PropsWithChildren} from "react";

export namespace Layout {
    export type Props = PropsWithChildren;
}

export default function Layout(
    {
        children,
    }: Layout.Props
) {
    return <RootLayout
        logo={logo}
    >
        {children}
    </RootLayout>;
}
