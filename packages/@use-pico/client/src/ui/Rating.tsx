import {
    type FC,
    type ReactNode
} from "react";

const twSize = {
    "xs": "",
    "sm": "",
    "md": "",
    "lg": "",
    "xl": "",
};
type twSize = typeof twSize;

export namespace Rating {
    export interface Props {
        defaultValue?: number;
        count?: number;
        readOnly?: boolean;
        size?: Size;
        emptySymbol?: (value: number) => ReactNode;
        fullSymbol?: (value: number) => ReactNode;
    }

    export type Size = keyof twSize;
}

export const Rating: FC<Rating.Props> = (
    {
        size = "md",
        count = 5,
    }
) => {
    return "Rating";
};
