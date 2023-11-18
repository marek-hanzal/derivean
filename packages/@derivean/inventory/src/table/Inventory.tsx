"use client";

import {ItemInline}        from "@derivean/item";
import {InventoryItemIcon} from "@derivean/ui";
import {t}                 from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                          from "react";

export namespace Inventory {
    export type Columns =
        | "item"
        | "amount"
        | "limit";

    export type Props =
        Omit<
            ComponentProps<typeof InventoryItemComponents.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            inventoryId?: string;
        }
}

export const Inventory: FC<Inventory.Props> = (
    {
        inventoryId,
        ...props
    }
) => {
    return <InventoryItemComponents.Table
        text={{
            total: t()`Total count of inventory items`,
            count: {
                empty: {
                    title:   t()`Empty inventory`,
                    message: t()`This inventory is empty`,
                },
            },
        }}
        name={"inventory.item"}
        icon={<InventoryItemIcon/>}
        columns={{
            item:   {
                title:  t()`Item`,
                render: ({item}) => <ItemUI.Fetch
                    override={item.itemId}
                    loader={<Loader size={"xs"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ItemInline entity={entity}/>}
                />,
            },
            amount: {
                title:  t()`Amount`,
                render: ({item}) => item.amount,
                width:  10,
            },
            limit:  {
                title:  t()`Limit`,
                render: ({item}) => item.limit,
                width:  10,
            },
        }}
        {...props}
    />;
};
