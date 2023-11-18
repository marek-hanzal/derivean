import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Alert {
    export type Props = PropsWithChildren<CommonProps & {
        icon?: ReactNode;
        color?: CommonProps.Color;
        title?: ReactNode;
    }>;
}

export const Alert: FC<Alert.Props> = () => {
    return "Alert";
};
