import {cn}    from "@use-pico/utils";
import {
    type FC,
    type HTMLAttributes,
    type PropsWithChildren,
    type ReactNode
}              from "react";
import {css}   from "../tools/css";
import {Title} from "./Title";

export namespace Page {
    export type Props =
        HTMLAttributes<HTMLDivElement>
        & PropsWithChildren<{
        /**
         * Icon in a page title
         */
        icon?: ReactNode;
        /**
         * All texts available in a page
         */
        text?: {
            /**
             * Header text (not a title)
             */
            header?: ReactNode;
        };
        /**
         * Append something to the page header (for example primary action button).
         */
        extra?: ReactNode;
        /**
         * Socket for navigation (could be basically anything - menu, breadcrumbs, etc.)
         */
        nav?: ReactNode;
        /**
         * Socket for menu (could be basically anything - menu, breadcrumbs, etc.)
         */
        menu?: ReactNode;
    }>;
}

/**
 * Page is a base component for all pages. It provides basic page layout.
 */
export const Page: FC<Page.Props> = (
    {
        icon,
        text,
        extra,
        nav,
        menu,
        children,
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            //
        ])}
        {...$props}
    >
        <div
            className={cn(
                "flex flex-row items-center",
                "border-b-1 border-zinc-100",
                "px-3 py-1",
                "gap-4",
            )}
        >
            {nav && <div className={"shrink"}>
                {nav}
            </div>}
            <div className={"grow"}>
                <Title
                    order={4}
                >
                    <div className={"flex flex-row items-center gap-2"}>
                        {icon && <div>
                            {icon}
                        </div>}
                        {text?.header}
                    </div>
                </Title>
            </div>
            {extra && <div className={"shrink"}>
                {extra}
            </div>}
        </div>
        {menu && <div className={"flex py-1 px-2"}>
            {menu}
        </div>}
        <div
            className={cn(
                "mt-4 px-12",
                "animate-fade-in animate-duration-200",
            )}
        >
            {children}
        </div>
    </div>;
};
