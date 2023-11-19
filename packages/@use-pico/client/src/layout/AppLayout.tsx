import {
    type FC,
    type PropsWithChildren
}                            from "react";
import {BlockLoadingOverlay} from "../ui/BlockLoadingOverlay";
import {Divider}             from "../ui/Divider";
import {Header}              from "../ui/Header";
import {Main}                from "../ui/Main";
import {Unblock}             from "../utils/Unblock";

export namespace AppLayout {
    export interface Props extends PropsWithChildren {
        header?: Header.Props;
    }
}

/**
 * General layout used inside app when a user is logged-in.
 */
export const AppLayout: FC<AppLayout.Props> = (
    {
        header,
        children
    }) => {
    return <>
        <Unblock/>
        <BlockLoadingOverlay/>
        {header && <>
            <Header
                {...header}
            />
            <Divider variant={"dotted"} my={"md"} mt={"xs"}/>
        </>}
        <Main>
            {children}
        </Main>
    </>;
};
