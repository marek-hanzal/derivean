import {IconX}                    from "@tabler/icons-react";
import {Translation}              from "@use-pico2/i18n";
import {type IBaseSelectionStore} from "@use-pico2/selection";
import {useStore}                 from "@use-pico2/store";
import {
    Button,
    ModalStore
}                                 from "@use-pico2/ui";
import {type FC}                  from "react";

export namespace ClearButton {
    export interface Props extends Button.Props {
        SelectionStore: IBaseSelectionStore<any>;
    }
}

export const ClearButton: FC<ClearButton.Props> = (
    {
        SelectionStore,
        ...props
    }
) => {
    const {close} = useStore(ModalStore, ({close}) => ({close}));
    const {clear} = useStore(SelectionStore, ({clear}) => ({clear}));

    return <Button
        leftSection={<IconX/>}
        variant={"subtle"}
        size={"md"}
        onClick={() => {
            clear();
            close("query-input");
        }}
        {...props}
    >
        <Translation namespace={"common"} label={"selection"} withLabel={"clear.button"}/>
    </Button>;
};
