"use client";

import {
    ItemInline,
    ItemUI
}                                from "@derivean/item";
import {
    InventoryIcon,
    InventoryItemIcon
}                                from "@derivean/ui";
import {t}                       from "@use-pico/i18n";
import {
    ButtonLink,
    Loader
}                                from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                                from "react";
import {InventoryItemUpsertForm} from "../form/InventoryItemUpsertForm";
import {InventoryInline}         from "../inline/InventoryInline";
import {InventoryItemRpc}        from "../rpc/InventoryItemRpc";
import {InventoryItemUI}         from "../ui/InventoryItemUI";
import {InventoryUI}             from "../ui/InventoryUI";

export namespace InventoryItemTable {
    export type Columns =
        | "inventory"
        | "item"
        | "amount"
        | "limit";

    export type Props =
        Omit<
            ComponentProps<typeof InventoryItemUI.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            inventoryId?: string;
        }
}

export const InventoryItemTable: FC<InventoryItemTable.Props> = (
    {
        inventoryId,
        ...props
    }
) => {
    return <InventoryItemUI.Table
        text={{
            total: t()`Total count of inventory items`,
            count: {
                empty: {
                    title:   t()`Empty inventory`,
                    message: t()`This inventory is empty`,
                },
            },
        }}
        name={"inventory item"}
        icon={<InventoryItemIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create inventory item (modal)`,
                    label: t()`Create inventory item`,
                },
            },
            upsertForm: ({modalId}) => <InventoryItemUpsertForm
                inventoryId={inventoryId}
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update inventory item`,
                    label: t()`Update inventory item`,
                },
                delete: {
                    label: t()`Delete inventory item`,
                    modal: {
                        title:   t()`Delete inventory item (modal)`,
                        content: t()`Do you really want to delete selected inventory item?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`InventoryItem has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: InventoryItemRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <InventoryItemUpsertForm
                inventoryId={inventoryId}
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            inventory: {
                title:  t()`Inventory`,
                render: ({item}) => <InventoryUI.Fetch
                    override={item.inventoryId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<InventoryIcon/>}
                        href={{
                            href:  "/manager/inventory/[id]",
                            query: {
                                id: item.id,
                            },
                        }}
                        label={<InventoryInline entity={entity}/>}
                    />}
                />,
            },
            item:      {
                title:  t()`Item`,
                render: ({item}) => <ItemUI.Fetch
                    override={item.itemId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ItemInline entity={entity}/>}
                />,
            },
            amount:    {
                title:  t()`Amount`,
                render: ({item}) => item.amount,
                width:  10,
            },
            limit:     {
                title:  t()`Limit`,
                render: ({item}) => item.limit,
                width:  10,
            },
        }}
        {...props}
    />;
};
