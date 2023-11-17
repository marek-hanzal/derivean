import {Divider}       from "@use-pico/client";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace GameLayout {
    export type Props = CoolPublicLayout.Props;
}

export const GameLayout: FC<GameLayout.Props> = props => {
    return <>
        <AppLayout
            homeUrl={"/game"}
            right={<SignOutButton/>}
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
