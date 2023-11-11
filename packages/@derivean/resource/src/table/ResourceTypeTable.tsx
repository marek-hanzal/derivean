"use client";

import {ResourceIcon}               from "@derivean/ui";
import {t}                          from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                                   from "react";
import {ResourceTypeUpsertForm}     from "../form/ResourceTypeUpsertForm";
import {ResourceTypeRpc}            from "../rpc/ResourceTypeRpc";
import {ResourceTypeSelectionStore} from "../store/ResourceTypeSelectionStore";
import {ResourceTypeUI}             from "../ui/ResourceTypeUI";

export namespace ResourceTypeTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        ComponentProps<typeof ResourceTypeUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const ResourceTypeTable: FC<ResourceTypeTable.Props> = props => {
    return <ResourceTypeUI.Table
        text={{
            total: t()`Resource type count`,
        }}
        name={"resource.type"}
        icon={<ResourceIcon/>}
        SelectionStore={ResourceTypeSelectionStore.single}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create resource type (modal)`,
                    label: t()`Create resource type`,
                }
            },
            upsertForm: ({modalId}) => <ResourceTypeUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update resource type`,
                    label: t()`Update resource type`,
                },
                delete: {
                    label: t()`Delete resource type`,
                    modal: {
                        title:   t()`Delete resource type (modal)`,
                        content: t()`Do you really want to delete selected resource type?`,
                        success: {
                            title: t()`Success`,
                            message: t()`Selected resource type has been successfully deleted.`,
                        },
                    }
                }
            },
            withMutation: ResourceTypeRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ResourceTypeUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                title: t()`Resource type name`,
                render: ({item}) => item.name,
            },
        }}
        {...props}
    />;
};
