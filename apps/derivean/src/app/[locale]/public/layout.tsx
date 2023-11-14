import {PublicLayout}           from "@derivean/public";
import {type PropsWithChildren} from "react";
import logo                     from "../../../../public/assets/logo/logo.svg";

export namespace Layout {
    export type Props = PropsWithChildren;
}

export default async function Layout(
    {
        children,
    }: Layout.Props
) {
    return <PublicLayout
        logo={logo}
        loginUrl={"/game"}
    >
        {children}
    </PublicLayout>;
}
