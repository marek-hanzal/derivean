import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";

export namespace Container {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Container: FC<Container.Props> = () => {
    return "Container";
};
