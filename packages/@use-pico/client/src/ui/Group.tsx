import {
    type FC,
    type ReactElement
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {GroupCol}         from "./Group/GroupCol";

export namespace Group {
    export type Group =
        CommonProps
        & {
            gap?: CommonProps.Size;
            children: ReactElement<GroupCol.Props> | ReactElement<GroupCol.Props>[];
        };
}

/**
 * Flex based Group; use GroupCol children to have more precise control over the layout.
 */
export const Group: FC<Group.Group> = (
    {
        children,
    }
) => {
    return <div className={"flex flex-row"}>
        {children}
    </div>;
};
