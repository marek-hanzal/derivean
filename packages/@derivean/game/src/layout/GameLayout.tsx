import {AppLayout}     from "@use-pico/client";
import {type FC}       from "react";
import {Footer}        from "../ui/Footer";
import {SignOutButton} from "./GameLayout/SignOutButton";

export namespace GameLayout {
    export type Props = Partial<AppLayout.Props>;
}

export const GameLayout: FC<GameLayout.Props> = (
    {
        header,
        ...props
    }
) => {
    return <>
        <AppLayout
            header={{
                home:  "/game",
                right: <SignOutButton/>,
                ...header,
            }}
            {...props}
        />
        <Footer
            mb={"xl"}
        />
    </>;
};
