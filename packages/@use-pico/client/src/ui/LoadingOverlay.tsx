import {type FC} from "react";

export namespace LoadingOverlay {
    export interface Props {
        visible?: boolean;
    }
}

export const LoadingOverlay: FC<LoadingOverlay.Props> = (
    {
        visible = true
    }
) => {
    return visible ? "LoadingOverlay" : null;
};
