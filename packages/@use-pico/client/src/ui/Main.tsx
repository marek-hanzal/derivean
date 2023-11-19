import {type FC}          from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Main {
    export type Props = CommonProps;
}

export const Main: FC<Main.Props> = (
    {
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <main
        className={cn([])}
        {...$props}
    />;
};
