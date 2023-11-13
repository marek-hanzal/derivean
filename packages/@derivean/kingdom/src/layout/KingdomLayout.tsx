import {Divider}       from "@use-pico/ui";
import {
    AppLayout,
    PublicLayout as CoolPublicLayout
}                      from "@use-pico/ui-extra";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./KingdomLayout/SignOutButton";

export namespace KingdomLayout {
    export type Props = CoolPublicLayout.Props;
}

export const KingdomLayout: FC<KingdomLayout.Props> = props => {
    return <>
        <AppLayout
            right={<SignOutButton/>}
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
