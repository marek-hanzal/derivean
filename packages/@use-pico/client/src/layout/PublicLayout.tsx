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
import {Group}               from "../ui/Group";
import {GroupCol}            from "../ui/Group/GroupCol";
import {LocaleLink}          from "../ui/LocaleLink";
import {Unblock}             from "../utils/Unblock";
import {SignInButton}        from "./PublicLayout/SignInButton";

export namespace PublicLayout {
    export interface Props extends PropsWithChildren {
        logo: ComponentProps<typeof Image>["src"];
        /**
         * Home url (usually on logo)
         */
        home?: string;
        /**
         * If provided, user will be redirected here; defaults to next-auth signIn()
         */
        loginUrl?: string;
        signInOptions?: SignInButton.Props["signInOptions"];
        /**
         * Hides login button from header
         */
        withoutLogin?: boolean;
        /**
         * Center part of the layout (header)
         */
        center?: ReactNode;
        right?: ReactNode;
    }
}

export const PublicLayout: FC<PublicLayout.Props> = (
    {
        logo,
        home = "/public",
        loginUrl,
        signInOptions,
        withoutLogin = false,
        center,
        right,
        children
    }) => {
    return <>
        <Unblock/>
        <BlockLoadingOverlay/>
        <Flex
            align={"center"}
            px={"sm"}
            pt={"sm"}
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
                <Group gap={"xs"}>
                    <GroupCol>
                        {right}
                    </GroupCol>
                    <GroupCol>
                        {!withoutLogin && <Group>
                            <GroupCol>
                                <SignInButton
                                    loginUrl={loginUrl}
                                    signInOptions={signInOptions}
                                />
                            </GroupCol>
                        </Group>}
                    </GroupCol>
                </Group>
            </FlexItem>
        </Flex>
        {children}
    </>;
};
