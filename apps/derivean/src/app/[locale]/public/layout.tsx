import {PublicLayout}           from "@derivean/public";
import {type PropsWithChildren} from "react";
import logo                     from "../../../../public/assets/logo/logo.svg";

export namespace Layout {
    export type Props = PropsWithChildren<{
        params: {
            locale: string;
        };
    }>;
}

export default function Layout(
    {
        children,
        params
    }: Layout.Props
) {
    return <PublicLayout
        logo={logo}
    >
        {children}
    </PublicLayout>;
}
