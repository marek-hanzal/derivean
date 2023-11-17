import {
    type FC,
    type PropsWithChildren
}                    from "react";
import {CommonProps} from "../../api/CommonProps";

export namespace GridCol {
    export type Props = PropsWithChildren<CommonProps & {
        span: GridCol.Span;
    }>;

    export type Span =
        | number
        | "auto"
        | "content";
}

export const GridCol: FC<GridCol.Props> = () => {
    return "GridCol";
};
