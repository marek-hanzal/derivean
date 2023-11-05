import {GameLayout}             from "@derivean/game";
import {tx}                     from "@use-pico/i18n";
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
    const foo = tx`Vlalkdfjk`;

    return <GameLayout
        logo={logo}
    >
        {children}
    </GameLayout>;
}
