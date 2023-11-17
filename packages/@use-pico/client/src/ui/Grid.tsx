import {
    type CSSProperties,
    type FC,
    type PropsWithChildren
}                    from "react";
import {CommonProps} from "../api/CommonProps";

export namespace Grid {
    export type Props = PropsWithChildren<CommonProps & {
        align?: CSSProperties["alignItems"];
    }>;
}

export const Grid: FC<Grid.Props> = () => {
    return "Grid";
};
