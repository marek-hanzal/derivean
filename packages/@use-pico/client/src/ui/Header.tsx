import Image              from "next/image";
import {
    type ComponentProps,
    type FC,
    type HtmlHTMLAttributes,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {type IHrefProps}  from "../api/IHrefProps";
import {tailwindify}      from "../tools/tailwindify";
import {LocaleLink}       from "./LocaleLink";

export namespace Header {
    export interface Props extends HtmlHTMLAttributes<HTMLDivElement>, CommonProps {
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
    const {
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([
            "px-3 py-2 mb-6",
            "bg-zinc-100",
            "shadow-md shadow-zinc-300",
            "flex flex-row items-center",
        ])}
        {...$props}
    >
        <div className={"shrink"}>
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
        </div>
        <div className={"flex-auto"}>
            {center}
        </div>
        <div className={"shrink"}>
            {right}
        </div>
    </div>;
};
