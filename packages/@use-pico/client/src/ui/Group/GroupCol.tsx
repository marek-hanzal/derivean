import {
    type FC,
    type PropsWithChildren
} from "react";

export namespace GroupCol {
    export type Props = PropsWithChildren;
}

export const GroupCol: FC<GroupCol.Props> = (
    {
        children,
    }
) => {
    return <div>
        {children}
    </div>;
};
