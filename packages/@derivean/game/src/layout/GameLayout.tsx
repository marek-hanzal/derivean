import {
    AppLayout,
    Divider
}                      from "@use-pico/client";
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
        <Divider variant={"dotted"} my={"md"} mt={"xs"}/>
        <Footer
            mb={"xl"}
        />
    </>;
};
