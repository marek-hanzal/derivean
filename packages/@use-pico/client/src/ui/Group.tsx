import {
    type FC,
    type HTMLAttributes,
    type ReactElement
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";
import {GroupCol}         from "./Group/GroupCol";

export namespace Group {
    export type Group = &
        HTMLAttributes<HTMLDivElement>
        &
        CommonProps
        & {
            /**
             * TailwindCSS gap property.
             */
            gap?: string;
            children: ReactElement<GroupCol.Props> | ReactElement<GroupCol.Props>[];
        };
}

/**
 * Flex based Group; use GroupCol children to have more precise control over the layout.
 */
export const Group: FC<Group.Group> = (
    {
        children,
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <div
        className={cn([
            "flex flex-row",
        ])}
        {...$props}
    >
        {children}
    </div>;
};
