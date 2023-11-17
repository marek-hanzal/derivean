import {
    type FC,
    PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Text {
    export type Props = PropsWithChildren<CommonProps & {
        c?: CommonProps.Color;
    }>;
}

export const Text: FC<Text.Props> = () => {
    return "Text";
};
