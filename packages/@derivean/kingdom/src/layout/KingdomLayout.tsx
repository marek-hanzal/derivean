import {
    AppLayout,
    Divider
}                      from "@use-pico/client";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./KingdomLayout/SignOutButton";

export namespace KingdomLayout {
    export type Props = AppLayout.Props;
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
