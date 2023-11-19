import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Main {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Main: FC<Main.Props> = (
    {
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <main
        className={cn([
            "min-h-screen",
        ])}
        {...$props}
    />;
};
