import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../../api/CommonProps";

export namespace GridCol {
    export type Props = PropsWithChildren<CommonProps & {
        span?: GridCol.Span;
    }>;

    export type Span =
        | number
        | "auto"
        | "content";
}

export const GridCol: FC<GridCol.Props> = (
    {
        children,
    }
) => {
    return <div>
        {children}
    </div>;
};
