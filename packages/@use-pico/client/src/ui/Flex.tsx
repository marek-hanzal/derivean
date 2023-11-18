import {cn}               from "@use-pico/utils";
import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";
import {twAlignItems}     from "../tools/tailwindify/twAlignItems";

export namespace Flex {
    export type Props = PropsWithChildren<CommonProps & {
        align?: Flex.AlignItems;
    }>;

    export type AlignItems = twAlignItems.Values;
}

export const Flex: FC<Flex.Props> = (
    {
        align,
        children,
        ...props
    },
) => {
    return <div
        className={cn(
            "flex",
            align ? twAlignItems[align] : undefined,
            ...tailwindify(props)
        )}
    >
        {children}
    </div>;
};
