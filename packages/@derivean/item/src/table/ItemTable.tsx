"use client";

import {ItemIcon}           from "@derivean/ui";
import {t}                  from "@use-pico/i18n";
import {
    Group,
    Text
}                           from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                           from "react";
import {ItemUpsertForm}     from "../form/ItemUpsertForm";
import {ItemInline}         from "../inline/ItemInline";
import {ItemTypeInline}     from "../inline/ItemTypeInline";
import {ItemRpc}            from "../rpc/ItemRpc";
import {ItemSelectionStore} from "../store/ItemSelectionStore";
import {ItemTypeUI}         from "../ui/ItemTypeUI";
import {ItemUI}             from "../ui/ItemUI";

export namespace ItemTable {
    export type Columns =
        | "name"
        | "typeId";

    export type Props =
        Omit<
            ComponentProps<typeof ItemUI.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            resourceTypeId?: string;
        }
}

export const ItemTable: FC<ItemTable.Props> = (
    {
        resourceTypeId,
        ...props
    }
) => {
    return <ItemUI.Table
        text={{
            total: t()`Item count`,
            count: {
                loading: {
                    title:    t()`Loading items`,
                    subtitle: t()`We're preparing item list for you...`,
                },
            },
        }}
        name={"item"}
        icon={<ItemIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create item (modal)`,
                    label: t()`Create item`,
                },
            },
            upsertForm: ({modalId}) => <ItemUpsertForm
                resourceTypeId={resourceTypeId}
                withAutoClose={[modalId]}
            />,
        }}
        SelectionStore={ItemSelectionStore.single}
        rowActionProps={{
            text:         {
                delete: {
                    label: t()`Delete item`,
                    modal: {
                        title:   t()`Delete item (modal)`,
                        content: t()`Do you really want to delete selected item?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Item has been successfully deleted.`,
                        },
                    }
                },
                update: {
                    title: t()`Update item`,
                    label: t()`Update item`,
                },
            },
            withMutation: ItemRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ItemUpsertForm
                resourceTypeId={resourceTypeId}
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name:   {
                title:  t()`Item name`,
                render: ({item}) => <Group gap={"xs"}>
                    <ItemInline entity={item}/>
                    <Text c={"dimmed"}>
                        ({item.name})
                    </Text>
                </Group>,
            },
            typeId: {
                title:      t()`Item type`,
                withFilter: {
                    isFilter: filter => filter?.typeId !== undefined,
                    onFilter: ({
                                   shallowFilter,
                                   item,
                               }) => {
                        shallowFilter({
                            typeId: item.typeId,
                        });
                    },
                    onClear:  ({shallowFilter}) => {
                        shallowFilter({
                            typeId: undefined,
                        });
                    },
                },
                render:     ({item}) => <ItemTypeUI.Fetch
                    override={item.typeId}
                    WithSuccess={ItemTypeInline}
                />,
                width:      16,
            },
        }}
        {...props}
    />;
};
