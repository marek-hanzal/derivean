import {StoreProvider} from "@use-pico2/store";
import {
    type FC,
    type PropsWithChildren
}                      from "react";
import {LoopStore}     from "./LoopStore";

export namespace LoopProvider {
    export type Props = PropsWithChildren;
}

export const LoopProvider: FC<LoopProvider.Props> = props => {
    return <StoreProvider
        values={{}}
        store={LoopStore}
        {...props}
    />;
};
