import {type FC} from "react";

export namespace Skeleton {
    export interface Props {
        lines?: number;
    }
}

export const Skeleton: FC<Skeleton.Props> = (
    {
        lines = 1,
    }
) => {
    return "Skeleton";
};
