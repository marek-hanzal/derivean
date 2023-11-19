import {
    type FC,
    HTMLAttributes
}                         from "react";
import {type CommonProps} from "../../api/CommonProps";
import {tailwindify}      from "../../tools/tailwindify";

const twSpan = {
    "content": "shrink",
    "auto":    "grow",
} as const;
type twSpan = typeof twSpan;

export namespace FlexItem {
    export type Props =
        HTMLAttributes<HTMLDivElement>
        & CommonProps
        & {
            span?: Span;
        };

    export type Span = keyof twSpan;
}

export const FlexItem: FC<FlexItem.Props> = (
    {
        span,
        children,
        ...props
    },
) => {
    const {cn, $props} = tailwindify(props);

    return <div
        className={cn([
            span ? twSpan[span] : undefined,
        ])}
        {...$props}
    >
        {children}
    </div>;
};
