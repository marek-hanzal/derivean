"use client";

import {InventoryIcon}       from "@derivean/ui";
import {t}                   from "@use-pico/i18n";
import {ButtonLink}          from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                            from "react";
import {InventoryUpsertForm} from "../form/InventoryUpsertForm";
import {InventoryInline}     from "../inline/InventoryInline";
import {InventoryRpc}        from "../rpc/InventoryRpc";
import {InventoryComponents} from "../ui/InventoryComponents";

export namespace InventoryTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        ComponentProps<typeof InventoryComponents.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const InventoryTable: FC<InventoryTable.Props> = props => {
    return <InventoryComponents.Table
        text={{
            total: t()`Total count of inventories`,
        }}
        name={"inventory"}
        icon={<InventoryIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create inventory (modal)`,
                    label: t()`Create inventory`,
                },
            },
            upsertForm: ({modalId}) => <InventoryUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update inventory`,
                    label: t()`Update inventory`,
                },
                delete: {
                    label: t()`Delete inventory`,
                    modal: {
                        title:   t()`Delete inventory (modal)`,
                        content: t()`Do you really want to delete selected inventory?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Inventory has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: InventoryRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <InventoryUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                title:  t()`Inventory name`,
                render: ({item}) => <ButtonLink
                    icon={<InventoryIcon/>}
                    href={{
                        href:  "/manager/inventory/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={<InventoryInline entity={item}/>}
                />,
            },
        }}
        {...props}
    />;
};
