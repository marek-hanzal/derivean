"use client";

import {ResourceIcon}           from "@derivean/ui";
import {withDullSchema}         from "@use-pico/dull-stuff";
import {t}                      from "@use-pico/i18n";
import {Table}                  from "@use-pico/ui-extra";
import {type FC}                from "react";
import {ResourceUpsertForm}     from "../form/ResourceUpsertForm";
import {ResourceTypeInline}     from "../inline/ResourceTypeInline";
import {ResourceQueryStore}     from "../query/ResourceQueryStore";
import {ResourceRpc}            from "../rpc/ResourceRpc";
import {ResourceSchema}         from "../schema/ResourceSchema";
import {ResourceSelectionStore} from "../store/ResourceSelectionStore";
import {ResourceTypeUI}         from "../ui/ResourceTypeUI";

export namespace ResourceTable {
    export type Columns =
        | "name"
        | "typeId";

    export type Props = Omit<
        Table.Props<
            Columns,
            withDullSchema.Infer.EntitySchema<ResourceSchema>,
            withDullSchema.Infer.QuerySchema<ResourceSchema>
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const ResourceTable: FC<ResourceTable.Props> = props => {
    return <Table
        text={{
            total: t()`Resource count`,
            count: {
                loading: {
                    title:    t()`Loading resources`,
                    subtitle: t()`We're preparing resource list for you...`,
                },
            },
        }}
        name={"resource"}
        icon={<ResourceIcon/>}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create new resource`,
                    label: t()`Create resource`,
                },
            },
            upsertForm: ({modalId}) => <ResourceUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        SelectionStore={ResourceSelectionStore.single}
        rowActionProps={{
            text:         {
                delete: {
                    label: t()`Delete resource`,
                    modal: {
                        title: t()`Delete resource (modal)`,
                        content: t()`Do you really want to delete selected resource?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Resource has been successfully deleted.`,
                        },
                    }
                },
                update: {
                    title: t()`Update resource`,
                    label: t()`Update resource`,
                },
            },
            withMutation: ResourceRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <ResourceUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name:   {
                title: t()`Resource name`,
                render: ({item}) => item.name,
            },
            typeId: {
                title:  t()`Resource type`,
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
                render: ({item}) => <ResourceTypeUI.Fetch
                    override={item.typeId}
                    WithSuccess={ResourceTypeInline}
                />,
                width:      16,
            },
        }}
        withQueryStore={ResourceQueryStore}
        withSourceQuery={ResourceRpc.query}
        {...props}
    />;
};
