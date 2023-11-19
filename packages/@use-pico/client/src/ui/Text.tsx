import {
    type FC,
    type HTMLAttributes
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Text {
    export type Props =
        HTMLAttributes<HTMLSpanElement>
        & CommonProps
        & {
            c?: string;
            fw?: number;
        };
}

export const Text: FC<Text.Props> = (
    {
        c,
        children,
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <span
        className={cn([
            c === "dimmed" ? "text-secondary-500" : c,
        ])}
        {...$props}
    >
        {children}
    </span>;
};
