import {
    type FC,
    type PropsWithChildren
} from "react";

export namespace GridCol {
    export type Props = PropsWithChildren<{
        span: GridCol.Span;
    }>;

    export type Span = number;
}

export const GridCol: FC<GridCol.Props> = () => {
    return "GridCol";
};
