import {
    type FC,
    type HTMLAttributes
}                             from "react";
import {css}                  from "../../tools/css";
import {Loader as CoolLoader} from "../Loader";

export namespace Loader {
    export interface Props extends HTMLAttributes<HTMLDivElement>, css.Style {
    }
}

export const Loader: FC<Loader.Props> = (
    {
        ...props
    },
) => {
    const {
        cx,
        $props
    } = css(props);

    return <div
        className={cx([
            "flex justify-center items-center m-14",
        ])}
        {...$props}
    >
        <CoolLoader
            loader={"i-svg-spinners:ring-resize"}
            cn={[
                "h-16 w-16",
                "text-zinc-300",
            ]}
        />
    </div>;
};
