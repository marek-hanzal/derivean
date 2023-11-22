import {type FC} from "react";

export namespace BoolInline {
    export interface Props {
        bool?: boolean | null;
        checkIcon?: string;
        unCheckIcon?: string;
        undefinedIcon?: string;
    }
}

export const BoolInline: FC<BoolInline.Props> = (
    {
        bool,
        checkIcon = "i-tabler-square-rounded-check",
        unCheckIcon = "i-tabler-square-rounded-x",
        undefinedIcon = "i-tabler-square-rounded",
    }) => {
    return <div
        className={bool !== undefined && bool !== null ? (bool ? checkIcon : unCheckIcon) : undefinedIcon}
    />;
};
