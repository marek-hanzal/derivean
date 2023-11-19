import {
    type FC,
    type HTMLAttributes
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

const twFw = {
    500: "font-medium",
    600: "font-semibold",
} as const;
type twFw = typeof twFw;

export namespace Text {
    export type Props =
        HTMLAttributes<HTMLSpanElement>
        & CommonProps
        & {
            c?: string;
            fw?: Fw;
        };

    export type Fw = keyof twFw;
}

export const Text: FC<Text.Props> = (
    {
        c,
        fw,
        children,
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <span
        className={cn([
            c === "dimmed" ? "text-secondary-500 font-light" : c,
            fw && twFw[fw],
        ])}
        {...$props}
    >
        {children}
    </span>;
};
