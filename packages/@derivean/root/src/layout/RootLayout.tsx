import {Divider}   from "@use-pico2/ui";
import {AppLayout} from "@use-pico2/ui-extra";
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
