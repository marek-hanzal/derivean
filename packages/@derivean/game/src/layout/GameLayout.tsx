import {Divider}       from "@use-pico2/ui";
import {
    AppLayout,
    PublicLayout as CoolPublicLayout
}                      from "@use-pico2/ui-extra";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace GameLayout {
    export type Props = CoolPublicLayout.Props;
}

export const GameLayout: FC<GameLayout.Props> = props => {
    return <>
        <AppLayout
            right={<SignOutButton/>}
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
