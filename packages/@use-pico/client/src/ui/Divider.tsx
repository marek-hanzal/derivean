import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Divider {
    export type Props = PropsWithChildren<CommonProps & {
        orientation?: Orientation;
    }>;

    export type Orientation =
        | "horizontal"
        | "vertical";
}

export const Divider: FC<Divider.Props> = () => {
    return "Divider";
};
