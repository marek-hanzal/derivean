import {type FC}    from "react";
import {MenuItem}   from "../menu/MenuItem";
import {ModalStore} from "./ModalStore";

export namespace ModalMenuItem {
    export interface Props extends MenuItem.Props {
        modalId: string;
    }
}

export const ModalMenuItem: FC<ModalMenuItem.Props> = (
    {
        modalId,
        ...props
    }) => {
    const {open} = ModalStore.use(({open}) => ({open}));
    return <MenuItem
        onClick={() => open(modalId)}
        {...props}
    />;
};
