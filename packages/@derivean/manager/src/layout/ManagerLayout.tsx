import {Divider}       from "@use-pico/ui";
import {
    AppLayout,
    PublicLayout as CoolPublicLayout
}                      from "@use-pico/ui-extra";
import {type FC}       from "react";
import {ManagerMenu}   from "../menu/ManagerMenu";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace ManagerLayout {
    export type Props = CoolPublicLayout.Props;
}

export const ManagerLayout: FC<ManagerLayout.Props> = props => {
    return <>
        <AppLayout
            right={<SignOutButton/>}
            center={<ManagerMenu/>}
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
