import {Translation}              from "@use-pico2/i18n";
import {type IBaseSelectionStore} from "@use-pico2/selection";
import {
    Button,
    CloseIcon,
    ModalStore
}                                 from "@use-pico2/ui";
import {type FC}                  from "react";

export namespace CancelButton {
    export interface Props extends Button.Props {
        SelectionStore: IBaseSelectionStore<any>;
    }
}

export const CancelButton: FC<CancelButton.Props> = (
    {
        SelectionStore,
        ...props
    }
) => {
    const {close} = ModalStore.use(({close}) => ({close}));
    const {cancel} = SelectionStore.use(({cancel}) => ({cancel}));
    return <Button
        leftSection={<CloseIcon/>}
        variant={"subtle"}
        size={"md"}
        onClick={() => {
            cancel();
            close("query-input");
        }}
        {...props}
    >
        <Translation namespace={"common"} label={"selection"} withLabel={"cancel.button"}/>
    </Button>;
};
