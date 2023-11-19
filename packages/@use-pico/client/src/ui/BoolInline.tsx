import {
    IconHelpSquareRounded,
    IconSquareRoundedCheck,
    IconSquareRoundedX
} from "@tabler/icons-react";
import {
    type FC,
    type ReactNode
} from "react";

export namespace BoolInline {
    export interface Props {
        bool?: boolean | null;
        checkIcon?: ReactNode;
        unCheckIcon?: ReactNode;
        undefinedIcon?: ReactNode;
    }
}

export const BoolInline: FC<BoolInline.Props> = (
    {
        bool,
        checkIcon = <IconSquareRoundedCheck/>,
        unCheckIcon = <IconSquareRoundedX/>,
        undefinedIcon = <IconHelpSquareRounded/>,
    }) => {
    return bool !== undefined && bool !== null ? (bool ? checkIcon : unCheckIcon) : undefinedIcon;
};
