import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}            from "react";
import {css} from "../tools/css";

export namespace Alert {
    export type Props = PropsWithChildren<css.Style & {
        icon?: ReactNode;
        color?: css.Color;
        title?: ReactNode;
    }>;
}

export const Alert: FC<Alert.Props> = () => {
    return "Alert";
};
