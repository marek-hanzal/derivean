"use client";

import {ResourceIcon}               from "@derivean/ui";
import {t}                          from "@use-pico/i18n";
import {Table}                      from "@use-pico/ui-extra";
import {type FC}                    from "react";
import {ResourceTypeUpsertForm}     from "../form/ResourceTypeUpsertForm";
import {withResourceTypeMutation}   from "../mutation/withResourceTypeMutation";
import {ResourceTypeQueryStore}     from "../query/type/ResourceTypeQueryStore";
import {withResourceTypeQuery}      from "../query/type/withResourceTypeQuery";
import {ResourceTypeQuerySchema}    from "../schema/type/ResourceTypeQuerySchema";
import {ResourceTypeSchema}         from "../schema/type/ResourceTypeSchema";
import {ResourceTypeSelectionStore} from "../store/ResourceTypeSelectionStore";

export namespace ResourceTypeTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            ResourceTypeSchema,
            ResourceTypeQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const ResourceTypeTable: FC<ResourceTypeTable.Props> = props => {
    return <Table
        text={{
            total: t()`Resource type count`,
        }}
        name={"resource.type"}
        icon={<ResourceIcon/>}
        SelectionStore={ResourceTypeSelectionStore}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create new resource type`,
                    label: t()`Create resource type`,
                }
            },
            upsertForm: ({modalId}) => <ResourceTypeUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text: {
                update: {
                    title: t()`Update resource type`,
                    label: t()`Update resource type`,
                },
                delete: {
                    title: t()`Delete resource type?`,
                    label: t()`Delete resource type`,
                    modal: {
                        content: t()`Do you really want to delete selected resource type?`,
                        success: {
                            title: t()`Success`,
                            message: t()`Selected resource type has been successfully deleted.`,
                        },
                    }
                }
            },
            withMutation: withResourceTypeMutation,
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
                render: ({item}) => item.name,
            },
        }}
        withQueryStore={ResourceTypeQueryStore}
        withSourceQuery={withResourceTypeQuery}
        {...props}
    />;
};
