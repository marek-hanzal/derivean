import logo                     from "@/derivean/assets/logo/logo.svg";
import {PublicLayout}           from "@derivean/public";
import {type PropsWithChildren} from "react";

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
    >
        {children}
    </PublicLayout>;
}
