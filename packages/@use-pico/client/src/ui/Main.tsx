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
        cn,
        $props
    } = css(props);

    return <main
        className={cn([
            "min-h-screen",
        ])}
        {...$props}
    />;
};
