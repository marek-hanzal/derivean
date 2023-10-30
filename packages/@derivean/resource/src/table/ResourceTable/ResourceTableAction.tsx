import {TableActionMenu}    from "@use-pico2/table";
import {
    CreateIcon,
    Modal,
    ModalMenuItem
}                           from "@use-pico2/ui";
import {type FC}            from "react";
import {ResourceUpsertForm} from "../../form/ResourceUpsertForm";

export namespace ResourceTableAction {
    export interface Props {
    }
}

export const ResourceTableAction: FC<ResourceTableAction.Props> = () => {
    return <>
        <Modal
            icon={<CreateIcon/>}
            modalId={"create"}
            title={"create.title"}
        >
            <ResourceUpsertForm/>
        </Modal>

        <TableActionMenu>
            <ModalMenuItem
                modalId={"create"}
                withLabel={"create.label"}
            />
        </TableActionMenu>
    </>;
};
