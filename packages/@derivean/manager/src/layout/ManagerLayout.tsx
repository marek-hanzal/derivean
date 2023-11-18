import {
    AppLayout,
    Divider
}                      from "@use-pico/client";
import {type FC}       from "react";
import {ManagerMenu}   from "../menu/ManagerMenu";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace ManagerLayout {
    export type Props = AppLayout.Props;
}

export const ManagerLayout: FC<ManagerLayout.Props> = props => {
    return <>
        <AppLayout
            home={"/manager"}
            right={<SignOutButton/>}
            center={<ManagerMenu/>}
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
