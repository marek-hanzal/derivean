import {ResourceIcon}                  from "@derivean/ui";
import {DeleteModal}                   from "@use-pico/source-ui";
import {TableRowActionMenu}            from "@use-pico/table";
import {type WithItem}                 from "@use-pico/types";
import {
    EditIcon,
    Modal,
    ModalMenuItem,
    TrashIcon
}                                      from "@use-pico/ui";
import {type FC}                       from "react";
import {ResourceUpsertForm}            from "../../form/ResourceUpsertForm";
import {withResourceMutation}          from "../../mutation/withResourceMutation";
import {type ResourceRepositorySchema} from "../../schema/ResourceRepositorySchema";

export namespace ResourceTableRowAction {
    export interface Props extends WithItem.Schema<ResourceRepositorySchema> {
    }
}

export const ResourceTableRowAction: FC<ResourceTableRowAction.Props> = (
    {
        item,
    }
) => {
    const deleteModalId = `resource.delete.${item.id}`;
    const updateModalId = `resource.update.${item.id}`;

    return <>
        <DeleteModal
            modalId={deleteModalId}
            entity={item}
            icon={<ResourceIcon/>}
            title={"delete.title"}
            withMutation={withResourceMutation}
        />
        <Modal
            modalId={updateModalId}
            icon={<ResourceIcon/>}
            title={"update.title"}
        >
            <ResourceUpsertForm
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
