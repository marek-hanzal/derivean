import {
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace WithIcon {
    export interface Props {
        icon: ReactNode;
        size?: CommonProps.Size;
        color?: CommonProps.Color;
    }
}

export const WithIcon: FC<WithIcon.Props> = (
    {
        icon,
        ...props
    }) => {
    return icon;
};
