import {BuildingIcon}         from "@derivean/ui";
import {DeleteModal}          from "@use-pico/source-ui";
import {TableRowActionMenu}   from "@use-pico/table";
import {type WithItem}        from "@use-pico/types";
import {
    EditIcon,
    Modal,
    ModalMenuItem,
    TrashIcon
}                             from "@use-pico/ui";
import {type FC}              from "react";
import {BuildingUpsertForm}   from "../../form/BuildingUpsertForm";
import {withBuildingMutation} from "../../mutation/withBuildingMutation";
import {type BuildingSchema}  from "../../schema/BuildingSchema";

export namespace BuildingTableRowAction {
    export interface Props extends WithItem.Schema<BuildingSchema> {
    }
}

export const BuildingTableRowAction: FC<BuildingTableRowAction.Props> = (
    {
        item,
    }
) => {
    const deleteModalId = `building.delete.${item.id}`;
    const updateModalId = `building.update.${item.id}`;

    return <>
        <DeleteModal
            modalId={deleteModalId}
            entity={item}
            icon={<BuildingIcon/>}
            title={"delete.title"}
            withMutation={withBuildingMutation}
        />
        <Modal
            modalId={updateModalId}
            icon={<BuildingIcon/>}
            title={"update.title"}
        >
            <BuildingUpsertForm
                withAutoClose={[updateModalId]}
                entity={item}
            />
        </Modal>
        <TableRowActionMenu>
            <ModalMenuItem
                leftSection={<EditIcon/>}
                modalId={updateModalId}
                withLabel={"update.label"}
            />
            <ModalMenuItem
                leftSection={<TrashIcon/>}
                modalId={deleteModalId}
                withLabel={"delete.label"}
                color={"red.5"}
            />
        </TableRowActionMenu>
    </>;
};
