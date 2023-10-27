import {ManagerLayout}          from "@derivean/manager";
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
    return <ManagerLayout
        logo={logo}
    >
        {children}
    </ManagerLayout>;
}
