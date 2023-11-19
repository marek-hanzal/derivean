import {IconWhirl}        from "@tabler/icons-react";
import {isString}         from "@use-pico/utils";
import {
    type ComponentProps,
    type FC,
    type HTMLAttributes
}                         from "react";
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

export namespace Loader {
    export interface Props<
        TIconProps extends IconProps = ComponentProps<typeof IconWhirl>,
    > {
        size?: CommonProps.Size | number;
        Icon?: FC<TIconProps>;
        sx?: {
            root?: HTMLAttributes<HTMLDivElement>;
            icon?: TIconProps;
        };
    }

    export interface IconProps {
        size?: number | string;
        className?: string;
    }
}

export const Loader: FC<Loader.Props> = (
    {
        size = 16,
        Icon = IconWhirl,
        sx,
    }
) => {
    const root = tailwindify(sx?.root || {});
    const icon = tailwindify(sx?.icon || {});

    return <div
        className={root.cn([
            "self-center opacity-100",
        ])}
        {...root.$props}
    >
        <Icon
            size={isString(size) ? (twSize[size] ?? size) : size}
            className={icon.cn([
                "text-primary-500 animate-spin"
            ])}
            {...icon.$props}
        />
    </div>;
};
