import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Group {
    export type Group = PropsWithChildren<CommonProps & {
        gap?: CommonProps.Size;
    }>;
}

export const Group: FC<Group.Group> = () => {
    return "Group";
};
