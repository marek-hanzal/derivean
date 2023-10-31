import {Divider}   from "@use-pico/ui";
import {AppLayout} from "@use-pico/ui-extra";
import {type FC}   from "react";
import {Footer}    from "../ui/Footer";

export namespace RootLayout {
    export type Props = AppLayout.Props;
}

export const RootLayout: FC<RootLayout.Props> = props => {
    return <>
        <AppLayout
            {...props}
        />
        <Divider variant={"dotted"} m={"lg"}/>
        <Footer/>
    </>;
};
