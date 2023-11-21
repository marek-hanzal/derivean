import {
    type FC,
    type HTMLAttributes,
    type PropsWithChildren,
    type ReactNode
}                    from "react";
import {tailwindify} from "../tools/tailwindify";
import {Title}       from "./Title";

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
        cn,
        $props
    } = tailwindify(props);

    return <div
        className={cn([])}
        {...$props}
    >
        <div
            className={`
                flex flex-row items-center
                shadow-sm shadow-zinc-200 
                px-3 py-3
                h-14
                gap-4
            `}
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
            className={"mt-4 px-2"}
        >
            {children}
        </div>
    </div>;
};
