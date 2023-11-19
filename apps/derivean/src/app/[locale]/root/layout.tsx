import {RootLayout}             from "@derivean/root";
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
    return <RootLayout
        header={{
            logo,
        }}
    >
        {children}
    </RootLayout>;
}
