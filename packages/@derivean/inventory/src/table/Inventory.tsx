"use client";

import {
    ItemFetch,
    ItemInline
}                           from "@derivean/item";
import {InventoryItemIcon}  from "@derivean/ui";
import {Loader}             from "@use-pico/client";
import {t}                  from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                           from "react";
import {InventoryItemTable} from "../ui/InventoryItemComponents";

export namespace Inventory {
    export type Columns =
        | "item"
        | "amount"
        | "limit";

    export type Props =
        Omit<
            ComponentProps<typeof InventoryItemTable<Columns>>,
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
    return <InventoryItemTable
        text={{
            total: t`Total count of inventory items`,
            empty: {
                title:   t`Empty inventory`,
                message: t`This inventory is empty`,
            },
        }}
        icon={<InventoryItemIcon/>}
        columns={{
            item:   {
                title:  t`Item`,
                render: ({item}) => <ItemFetch
                    override={item.itemId}
                    loader={<Loader size={"xs"}/>}
                    WithSuccess={({entity}) => <ItemInline entity={entity}/>}
                />,
            },
            amount: {
                title: t`Amount`,
                render: ({item}) => item.amount,
                width: "w-10",
            },
            limit:  {
                title: t`Limit`,
                render: ({item}) => item.limit,
                width: "w-10",
            },
        }}
        {...props}
    />;
};
