import {type FC} from "react";
import {Loader}  from "./Loader";

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
    return visible ? <div
        className={"fixed inset-0 bg-gradient-to-r from-gray-50 to-gray-100 opacity-75 transition-opacity flex justify-center"}
    >
        <Loader
            size={64}
            sx={{
                root: {
                    className: "animate-bounce self-center",
                },
            }}
        />
    </div> : null;
};
