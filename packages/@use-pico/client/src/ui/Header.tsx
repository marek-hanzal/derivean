import Image              from "next/image";
import {
    type ComponentProps,
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {type IHrefProps}  from "../api/IHrefProps";
import {Flex}             from "./Flex";
import {FlexItem}         from "./Flex/FlexItem";
import {LocaleLink}       from "./LocaleLink";

export namespace Header {
    export interface Props extends CommonProps {
        /**
         * Optional logo on the left side
         */
        logo?: ComponentProps<typeof Image>["src"];
        /**
         * Home link (logo link)
         */
        home?: IHrefProps | string;
        /**
         * Center part of the layout (header)
         */
        center?: ReactNode;
        right?: ReactNode;
    }
}

export const Header: FC<Header.Props> = (
    {
        logo,
        home,
        center,
        right,
        ...props
    }
) => {
    return <Flex
        align={"center"}
        px={"md"}
        pt={"xs"}
        {...props}
    >
        <FlexItem span={"content"}>
            {logo && home && <LocaleLink
                href={home}
                style={{
                    display: "block",
                }}
            >
                <Image
                    priority
                    height={32}
                    alt={"logo"}
                    src={logo}
                />
            </LocaleLink>}
            {logo && !home && <Image
                priority
                height={32}
                alt={"logo"}
                src={logo}
            />}
        </FlexItem>
        <FlexItem span={"auto"}>
            {center}
        </FlexItem>
        <FlexItem span={"content"}>
            {right}
        </FlexItem>
    </Flex>;
};
