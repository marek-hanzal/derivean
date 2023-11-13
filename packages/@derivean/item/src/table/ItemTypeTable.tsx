"use client";

import {ItemIcon}               from "@derivean/ui";
import {t}                      from "@use-pico/i18n";
import {ButtonLink}             from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                               from "react";
import {ItemTypeUpsertForm}     from "../form/ItemTypeUpsertForm";
import {ItemTypeRpc}            from "../rpc/ItemTypeRpc";
import {ItemTypeSelectionStore} from "../store/ItemTypeSelectionStore";
import {ItemTypeUI}             from "../ui/ItemTypeUI";

export namespace ItemTypeTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        ComponentProps<typeof ItemTypeUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const ItemTypeTable: FC<ItemTypeTable.Props> = props => {
    return <ItemTypeUI.Table
        text={{
            total: t()`Item type count`,
            count: {
                empty: {
                    title:   t()`No item types found`,
                    message: t()`There are no item types yet. They're needed to create items itself.`,
                },
            },
        }}
        name={"item.type"}
        icon={<ItemIcon/>}
        SelectionStore={ItemTypeSelectionStore.single}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create item type (modal)`,
                    label: t()`Create item type`,
                }
            },
            upsertForm: ({modalId}) => <ItemTypeUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update item type`,
                    label: t()`Update item type`,
                },
                delete: {
                    label: t()`Delete item type`,
                    modal: {
                        title:   t()`Delete item type (modal)`,
                        content: t()`Do you really want to delete selected item type?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Selected item type has been successfully deleted.`,
                        },
                    }
                }
            },
            withMutation: ItemTypeRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ItemTypeUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                title:  t()`Item type name`,
                render: ({item}) => <ButtonLink
                    href={{
                        href:  `/manager/item/type/[id]`,
                        query: {id: item.id},
                    }}
                    label={item.name}
                />,
            },
        }}
        {...props}
    />;
};
