import {isString}         from "@use-pico/utils";
import {type FC}          from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twSize: Record<CommonProps.Size, number> = {
    "xs":  8,
    "sm":  16,
    "md":  24,
    "lg":  32,
    "xl":  64,
    "xl2": 92,
} as const;

export namespace Icon {
    export type Props<
        TIconProps extends IconProps
    > =
        TIconProps
        & CommonProps
        & {
            color?: string;
            size?: CommonProps.Size | number;
            Icon: FC<TIconProps>;
        }

    export interface IconProps {
        size?: number | string;
        className?: string;
    }
}

export const Icon = <
    TIconProps extends Icon.IconProps
>(
    {
        color,
        size = "md",
        Icon,
        ...props
    }: Icon.Props<TIconProps>
) => {
    const {cn, $props} = tailwindify(props);

    return <Icon
        size={isString(size) ? (twSize[size] ?? size) : size}
        className={cn([
            color,
        ])}
        {...$props as any}
    />;
};
