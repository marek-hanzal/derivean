import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace ActionIcon {
    export type Props = PropsWithChildren<CommonProps & {
        variant?: CommonProps.Variant;
        size?: CommonProps.Size;

        /**
         * @TODO extend from HTML element or use direct onClick type from an HTML element
         */
        onClick?: () => void;
    }>;
}

export const ActionIcon: FC<ActionIcon.Props> = () => {
    return "ActionIcon";
};
