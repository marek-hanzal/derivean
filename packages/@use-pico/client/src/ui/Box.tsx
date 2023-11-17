import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Box {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Box: FC<Box.Props> = () => {
    return "Box";
};
