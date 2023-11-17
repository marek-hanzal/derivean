import {
    type FC,
    type PropsWithChildren
} from "react";

export namespace Center {
    export type Props = PropsWithChildren;
}

export const Center: FC<Center.Props> = () => {
    return "Center";
};
