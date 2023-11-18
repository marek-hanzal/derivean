import {IconWhirl} from "@tabler/icons-react";
import {type FC}   from "react";

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
        <div className={"animate-bounce self-center opacity-100"}>
            <IconWhirl size={64} className={"text-primary-500 animate-spin"}/>
        </div>
    </div> : null;
};
