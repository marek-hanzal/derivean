"use client";

import {type FC}        from "react";
import {useStore$}      from "../hook/useStore$";
import {BlockStore}     from "../store/BlockStore";
import {LoadingOverlay} from "./LoadingOverlay";

export namespace BlockLoadingOverlay {
    export interface Props extends Omit<LoadingOverlay.Props, "visible"> {
    }
}

export const BlockLoadingOverlay: FC<BlockLoadingOverlay.Props> = props => {
    const block = useStore$(BlockStore, ({isBlock}) => ({isBlock}));
    return <LoadingOverlay
        // visible={block?.isBlock}
        visible
        {...props}
    />;
};
