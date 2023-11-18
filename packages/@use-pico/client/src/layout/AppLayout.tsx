import Image                 from "next/image";
import {
    type ComponentProps,
    type FC,
    type PropsWithChildren,
    type ReactNode
}                            from "react";
import {BlockLoadingOverlay} from "../ui/BlockLoadingOverlay";
import {Grid}                from "../ui/Grid";
import {GridCol}             from "../ui/Grid/GridCol";
import {Group}               from "../ui/Group";
import {GroupCol}            from "../ui/Group/GroupCol";
import {LocaleLink}          from "../ui/LocaleLink";
import {Unblock}             from "../utils/Unblock";

export namespace AppLayout {
    export interface Props extends PropsWithChildren {
        logo: ComponentProps<typeof Image>["src"];
        home?: string;
        /**
         * Center part of the layout (header)
         */
        center?: ReactNode;
        right?: ReactNode;
    }
}

/**
 * General layout used inside app when a user is logged-in.
 */
export const AppLayout: FC<AppLayout.Props> = (
    {
        logo,
        home = "/root",
        center,
        right,
        children
    }) => {
    return <>
        <Unblock/>
        <BlockLoadingOverlay/>
        <Grid
            align={"center"}
            px={"md"}
            pt={"xs"}
        >
            <GridCol span={"content"}>
                <LocaleLink
                    href={home}
                    style={{
                        display: "block",
                    }}
                >
                    <Image
                        priority={true}
                        height={32}
                        alt={"logo"}
                        src={logo}
                    />
                </LocaleLink>
            </GridCol>
            <GridCol span={"auto"} m={0}>
                {center}
            </GridCol>
            <GridCol span={"content"}>
                <Group gap={"xs"}>
                    <GroupCol>
                        {right}
                    </GroupCol>
                </Group>
            </GridCol>
        </Grid>
        {children}
    </>;
};
