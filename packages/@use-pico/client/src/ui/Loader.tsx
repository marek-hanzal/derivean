import {IconWhirl}           from "@tabler/icons-react";
import {type HTMLAttributes} from "react";
import {tailwindify}         from "../tools/tailwindify";
import {Icon}                from "./Icon";

export namespace Loader {
    export type Props<
        TIconProps extends Icon.IconProps,
    > =
        Partial<Icon.Props<TIconProps>>
        & {
            sx?: {
                root?: HTMLAttributes<HTMLDivElement>;
            };
        }
}

export const Loader = <
    TIconProps extends Icon.IconProps,
>(
    {
        sx,
        Icon: $Icon,
        ...   props
    }: Loader.Props<TIconProps>,
) => {
    const root = tailwindify(sx?.root || {});

    return <div
        className={root.cn([
            "self-center opacity-100",
        ])}
        {...root.$props}
    >
        <Icon
            size={"sm"}
            cn={[
                "text-primary-500 animate-spin",
            ]}
            Icon={$Icon || IconWhirl as Icon.Props<any>}
            {...props}
        />
    </div>;
};
