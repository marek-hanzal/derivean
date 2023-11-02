import {ResourceIcon}       from "@derivean/ui";
import {TableActionMenu}    from "@use-pico/table";
import {
    CreateIcon,
    Modal,
    ModalMenuItem
}                           from "@use-pico/ui";
import {type FC}            from "react";
import {ResourceUpsertForm} from "../../form/ResourceUpsertForm";

export namespace ResourceTableAction {
    export interface Props {
    }
}

export const ResourceTableAction: FC<ResourceTableAction.Props> = () => {
    const createId = "resource.create";
    return <>
        <Modal
            icon={<CreateIcon/>}
            modalId={createId}
            title={"create.title"}
        >
            <ResourceUpsertForm
                withAutoClose={[createId]}
            />
        </Modal>

        <TableActionMenu>
            <ModalMenuItem
                modalId={createId}
                withLabel={"create.label"}
                leftSection={<ResourceIcon/>}
            />
        </TableActionMenu>
    </>;
};
