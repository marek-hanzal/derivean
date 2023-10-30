"use client";

import {
    AppShell,
    Box,
    LoadingOverlay
}                      from "@mantine/core";
import {
    LocaleLink,
    useLocaleRouter
}                      from "@use-pico2/i18n";
import {useClearCache} from "@use-pico2/query";
import {useStore$}     from "@use-pico2/store";
import {
    BlockStore,
    Grid,
    GridCol,
    Group
}                      from "@use-pico2/ui";
import Image           from "next/image";
import {
    type ComponentProps,
    type FC,
    type PropsWithChildren,
    type ReactNode,
    useEffect
}                      from "react";

export namespace AppLayout {
    export interface Props extends PropsWithChildren {
        logo: ComponentProps<typeof Image>["src"];
        homeUrl?: string;
        publicUrl?: string;
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
        homeUrl = "/root",
        publicUrl = "/public",
        center,
        right,
        children
    }) => {
    const clearCache = useClearCache();
    const router = useLocaleRouter();
    const block = useStore$(BlockStore, ({
                                             unblock,
                                             isBlock
                                         }) => ({
        unblock,
        isBlock
    }));
    useEffect(() => {
        block?.unblock();
    }, []);
    return <AppShell>
        <Box>
            <LoadingOverlay
                visible={block?.isBlock || false}
            />
            <Grid
                align={"center"}
                px={"md"}
                pt={"xs"}
            >
                <GridCol span={"content"}>
                    <LocaleLink
                        href={homeUrl}
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
                        {right}
                    </Group>
                </GridCol>
            </Grid>
        </Box>
        <AppShell.Main>
            {children}
        </AppShell.Main>
    </AppShell>;
};
