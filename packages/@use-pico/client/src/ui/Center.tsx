import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Center {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Center: FC<Center.Props> = () => {
    return "Center";
};
