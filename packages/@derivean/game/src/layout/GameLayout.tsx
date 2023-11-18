import {
    AppLayout,
    Divider
}                      from "@use-pico/client";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace GameLayout {
    export type Props = AppLayout.Props;
}

export const GameLayout: FC<GameLayout.Props> = props => {
    return <>
        <AppLayout
            home={"/game"}
            right={<SignOutButton/>}
            {...props}
        />
        <Divider variant={"dotted"} my={"lg"}/>
        <Footer/>
    </>;
};
