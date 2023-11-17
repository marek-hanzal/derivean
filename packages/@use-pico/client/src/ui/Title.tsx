import {type FC} from "react";
import {Text}    from "./Text";

export namespace Title {
    export interface Props extends Text.Props {
        order?: number;
    }
}

export const Title: FC<Title.Props> = (
    {
        order = 1,
        ...props
    }
) => {
    return <Text
        {...props}
    />;
};
