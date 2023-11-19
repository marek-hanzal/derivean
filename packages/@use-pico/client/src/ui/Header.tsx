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
        logo?: {
            src: ComponentProps<typeof Image>["src"];
            link?: IHrefProps | string;
        };
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
            {logo && logo.link && <LocaleLink
                href={logo.link}
                style={{
                    display: "block",
                }}
            >
                <Image
                    height={32}
                    alt={"logo"}
                    src={logo.src}
                />
            </LocaleLink>}
            {logo && !logo.link && <Image
                height={32}
                alt={"logo"}
                src={logo.src}
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
