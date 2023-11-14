"use client";

import {
    ItemInline,
    ItemUI
}                          from "@derivean/item";
import {InventoryItemIcon} from "@derivean/ui";
import {t}                 from "@use-pico/i18n";
import {Loader}            from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                          from "react";
import {InventoryItemUI}   from "../ui/InventoryItemUI";

export namespace Inventory {
    export type Columns =
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

export const Inventory: FC<Inventory.Props> = (
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
