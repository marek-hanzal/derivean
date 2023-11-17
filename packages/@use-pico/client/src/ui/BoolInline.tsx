import {
    IconHelpSquareRounded,
    IconSquareRoundedCheck,
    IconSquareRoundedX
}                         from "@tabler/icons-react";
import {
    type FC,
    type ReactNode
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {WithIcon}         from "./WithIcon";

export namespace BoolInline {
    export interface Props {
        bool?: boolean | null;
        checkIcon?: ReactNode;
        checkColor?: CommonProps.Color;
        unCheckIcon?: ReactNode;
        unCheckColor?: CommonProps.Color;
        undefinedIcon?: ReactNode;
        undefinedColor?: CommonProps.Color;
    }
}

export const BoolInline: FC<BoolInline.Props> = (
    {
        bool,
        checkIcon = <IconSquareRoundedCheck/>,
        checkColor = "green",
        unCheckIcon = <IconSquareRoundedX/>,
        unCheckColor = "red",
        undefinedIcon = <IconHelpSquareRounded/>,
        undefinedColor = "blue",
    }) => {
    return bool !== undefined && bool !== null ? (bool ? <WithIcon
        icon={checkIcon}
        color={checkColor}
    /> : <WithIcon
        icon={unCheckIcon}
        color={unCheckColor}
    />) : <WithIcon
        icon={undefinedIcon}
        color={undefinedColor}
    />;
};
