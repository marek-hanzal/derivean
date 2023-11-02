import {ResourceIcon}         from "@derivean/ui";
import {DeleteModal}          from "@use-pico/source-ui";
import {TableRowActionMenu}   from "@use-pico/table";
import {type WithItem}        from "@use-pico/types";
import {
    ModalMenuItem,
    TrashIcon
}                             from "@use-pico/ui";
import {type FC}              from "react";
import {withResourceMutation} from "../../mutation/withResourceMutation";
import {type ResourceSchema}  from "../../schema/ResourceSchema";

export namespace ResourceTableRowAction {
    export interface Props extends WithItem.Schema<ResourceSchema> {
    }
}

export const ResourceTableRowAction: FC<ResourceTableRowAction.Props> = (
    {
        item,
    }
) => {
    const deleteModalId = `resource.delete.${item.id}`;

    return <>
        <DeleteModal
            modalId={deleteModalId}
            entity={item}
            icon={<ResourceIcon/>}
            title={"delete.title"}
            withMutation={withResourceMutation}
        />
        <TableRowActionMenu>
            <ModalMenuItem
                leftSection={<TrashIcon/>}
                modalId={deleteModalId}
                withLabel={"delete.label"}
                color={"red.5"}
            />
        </TableRowActionMenu>
    </>;
};
