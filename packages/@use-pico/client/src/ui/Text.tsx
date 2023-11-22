import {
    type FC,
    type HTMLAttributes
}            from "react";
import {css} from "../tools/css";

const twFw = {
    500: "font-medium",
    600: "font-semibold",
} as const;
type twFw = typeof twFw;

export namespace Text {
    export type Props =
        HTMLAttributes<HTMLSpanElement>
        & css.Style
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
    const {
        cn,
        $props
    } = css(props);

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
