import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Text {
    export type Props = PropsWithChildren<CommonProps & {
        c?: CommonProps.Color;
        fw?: number;
    }>;
}

export const Text: FC<Text.Props> = () => {
    return "Text";
};
