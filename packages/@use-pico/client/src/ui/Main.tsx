import {
    type FC,
    type PropsWithChildren
}            from "react";
import {css} from "../tools/css";

export namespace Main {
    export type Props = PropsWithChildren<css.Style>;
}

export const Main: FC<Main.Props> = (
    {
        ...props
    }
) => {
    const {
        cx,
        $props
    } = css(props);

    return <main
        className={cx([
            "min-h-screen",
        ])}
        {...$props}
    />;
};
