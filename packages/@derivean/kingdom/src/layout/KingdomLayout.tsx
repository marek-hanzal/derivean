import {AppLayout}     from "@use-pico/client";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./KingdomLayout/SignOutButton";

export namespace KingdomLayout {
    export type Props = AppLayout.Props;
}

export const KingdomLayout: FC<KingdomLayout.Props> = (
    {
        header,
        ...props
    }
) => {
    return <>
        <AppLayout
            header={{
                right: <SignOutButton/>,
                ...header,
            }}
            {...props}
        />
        <Footer/>
    </>;
};
