import {
    type FC,
    type HTMLAttributes
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";
import {twAlignItems}     from "../tools/tailwindify/twAlignItems";

export namespace Flex {
    export type Props =
        HTMLAttributes<HTMLDivElement>
        & CommonProps
        & {
            align?: Flex.AlignItems;
        };

    export type AlignItems = twAlignItems.Values;
}

export const Flex: FC<Flex.Props> = (
    {
        align,
        children,
        ...props
    },
) => {
    const {cn, $props} = tailwindify(props);

    return <div
        className={cn([
            "flex",
            align ? twAlignItems[align] : undefined,
        ])}
        {...$props}
    >
        {children}
    </div>;
};
