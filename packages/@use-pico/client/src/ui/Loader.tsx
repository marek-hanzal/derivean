import {type FC}          from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Loader {
    export interface Props {
        size?: CommonProps.Size;
        type?: Type;
    }

    export type Type =
        | "dots";
}

export const Loader: FC<Loader.Props> = (
    {
        type = "dots",
        size = "sm",
    }
) => {
    return "Loader";
};
