import {
    type FC,
    type PropsWithChildren,
    type ReactNode
} from "react";

export namespace Page {
    export type Props = PropsWithChildren<{
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
         * Postfix for a page (usually a navigation)
         */
        postfix?: ReactNode;
        /**
         * Append "something" at the bottom of page header (usually a menu)
         */
        append?: ReactNode;
    }>;
}

/**
 * Page is a base component for all pages. It provides basic page layout.
 */
export const Page: FC<Page.Props> = (
    {
        icon,
        text,
        children
    }
) => {
    return <div>
        {children}
    </div>;
};
