import {type IWithMutation} from "@use-pico/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                           from "@use-pico/schema";
import {DeleteModal}        from "@use-pico/source-ui";
import {TableRowActionMenu} from "@use-pico/table";
import {type WithItem}      from "@use-pico/types";
import {
    EditIcon,
    Modal,
    ModalMenuItem,
    TrashIcon
}                           from "@use-pico/ui";
import {type ReactNode}     from "react";

export namespace RowAction {
    export interface Props<
        TItemSchema extends WithIdentitySchema,
    > extends WithItem.Schema<TItemSchema> {
        label: {
            deleteModal: DeleteModal.Props<any, any>["label"];
        };
        name: string;
        icon: ReactNode;
        withMutation: IWithMutation<any, any>;
        upsertForm: UpsertFormFactory<TItemSchema>;
    }

    export type UpsertFormFactory<
        TItemSchema extends WithIdentitySchema,
    > = (props: UpsertFormFactory.Props<TItemSchema>) => ReactNode;

    export namespace UpsertFormFactory {
        export interface Props<
            TItemSchema extends WithIdentitySchema,
        > {
            item: PicoSchema.Output<TItemSchema>;
            modalId: string;
        }
    }
}

export const RowAction = <
    TItemSchema extends WithIdentitySchema,
>(
    {
        label,
        name,
        icon,
        withMutation,
        upsertForm,
        item,
    }: RowAction.Props<TItemSchema>
) => {
    const deleteModalId = `${name}.delete.${item.id}`;
    const updateModalId = `${name}.update.${item.id}`;

    return <>
        <DeleteModal
            label={label.deleteModal}
            modalId={deleteModalId}
            entity={item}
            icon={icon}
            title={"delete.title"}
            withMutation={withMutation}
        />
        <Modal
            modalId={updateModalId}
            icon={icon}
            title={"update.title"}
            modalProps={{
                closeOnClickOutside: false,
            }}
        >
            {upsertForm({
                item,
                modalId: updateModalId,
            })}
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
