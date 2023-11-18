import Image                 from "next/image";
import {
    type ComponentProps,
    type FC,
    type PropsWithChildren,
    type ReactNode
}                            from "react";
import {BlockLoadingOverlay} from "../ui/BlockLoadingOverlay";
import {Flex}                from "../ui/Flex";
import {FlexItem}            from "../ui/Flex/FlexItem";
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
        <Flex
            align={"center"}
            px={"md"}
            pt={"xs"}
        >
            <FlexItem span={"content"}>
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
            </FlexItem>
            <FlexItem span={"auto"}>
                {center}
            </FlexItem>
            <FlexItem span={"content"}>
                {right}
            </FlexItem>
        </Flex>
        {children}
    </>;
};
