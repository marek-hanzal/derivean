import {AppLayout} from "@use-pico/client";
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
        <Footer/>
    </>;
};
