import {
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Rating {
    export interface Props {
        defaultValue?: number;
        count?: number;
        readOnly?: boolean;
        size?: CommonProps.Size;
        emptySymbol?: (value: number) => ReactNode;
        fullSymbol?: (value: number) => ReactNode;
    }
}

export const Rating: FC<Rating.Props> = (
    {
        count = 5,
    }
) => {
    return "Rating";
};
