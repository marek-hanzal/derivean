import {BuildingIcon}       from "@derivean/ui";
import {TableActionMenu}    from "@use-pico/table";
import {
    CreateIcon,
    Modal,
    ModalMenuItem
}                           from "@use-pico/ui";
import {type FC}            from "react";
import {BuildingUpsertForm} from "../../form/BuildingUpsertForm";

export namespace BuildingTableAction {
    export interface Props {
    }
}

export const BuildingTableAction: FC<BuildingTableAction.Props> = () => {
    const createId = "building.create";
    return <>
        <Modal
            icon={<CreateIcon/>}
            modalId={createId}
            title={"create.title"}
        >
            <BuildingUpsertForm
                withAutoClose={[createId]}
            />
        </Modal>

        <TableActionMenu>
            <ModalMenuItem
                modalId={createId}
                withLabel={"create.label"}
                leftSection={<BuildingIcon/>}
            />
        </TableActionMenu>
    </>;
};
