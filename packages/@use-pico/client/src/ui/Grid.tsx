import {cn}               from "@use-pico/utils";
import {
    type CSSProperties,
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

const twGap = {
    "xs": "gap-1",
    "sm": "gap-2",
    "md": "gap-4",
    "lg": "gap-6",
    "xl": "gap-8",
} as const;

const twCols = {
    1:  "grid-cols-1",
    2:  "grid-cols-2",
    3:  "grid-cols-3",
    4:  "grid-cols-4",
    5:  "grid-cols-5",
    6:  "grid-cols-6",
    7:  "grid-cols-7",
    8:  "grid-cols-8",
    9:  "grid-cols-9",
    10: "grid-cols-10",
    11: "grid-cols-11",
    12: "grid-cols-12",
} as const;

export namespace Grid {
    export type Props = PropsWithChildren<CommonProps & {
        align?: CSSProperties["alignItems"];
        gap?: CommonProps.Size;
        cols?: Cols;
    }>;

    export type Cols = keyof typeof twCols;
}

export const Grid: FC<Grid.Props> = (
    {
        gap,
        cols,
        children,
    }
) => {
    return <div
        className={cn(
            "grid auto-cols-max",
            cols ? twCols[cols] : "grid-flow-col",
            gap ? (twGap[gap as keyof typeof twGap] ?? gap) : undefined
        )}
    >
        {children}
    </div>;
};
