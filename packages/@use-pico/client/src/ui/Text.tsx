import {cn}               from "@use-pico/utils";
import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Text {
    export type Props = PropsWithChildren<CommonProps & {
        c?: string;
        fw?: number;
    }>;
}

export const Text: FC<Text.Props> = (
    {
        c,
        children,
        ...props
    }
) => {
    return <span
        className={cn(
            "",
            c === "dimmed" ? "text-secondary-500" : c,
            tailwindify(props),
        )}
    >
        {children}
    </span>;
};
