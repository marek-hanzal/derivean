import {GameLayout}             from "@derivean/game";
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
    }: Layout.Props
) {
    return <GameLayout
        header={{
            logo,
        }}
    >
        {children}
    </GameLayout>;
}
