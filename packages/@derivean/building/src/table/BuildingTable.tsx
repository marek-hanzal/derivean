"use client";

import {BuildingIcon}             from "@derivean/ui";
import {tx}                       from "@use-pico/i18n";
import {ButtonLink}               from "@use-pico/ui";
import {Table}                    from "@use-pico/ui-extra";
import {type FC}                  from "react";
import {BuildingUpsertForm}       from "../form/BuildingUpsertForm";
import {withBuildingMutation}     from "../mutation/withBuildingMutation";
import {BuildingQueryStore}       from "../query/BuildingQueryStore";
import {withBuildingQuery}        from "../query/withBuildingQuery";
import {type BuildingQuerySchema} from "../schema/BuildingQuerySchema";
import {type BuildingSchema}      from "../schema/BuildingSchema";

export namespace BuildingTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            BuildingSchema,
            BuildingQuerySchema
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <Table
        text={{
            total: tx()`Total count of buildings`,
        }}
        name={"building"}
        icon={<BuildingIcon/>}
        tableActionProps={{
            upsertForm: ({modalId}) => <BuildingUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text: {
                deleteModal: {
                    content: tx()`Do you really want to delete selected building?`,
                    success: {
                        title:   tx()`Success`,
                        message: tx()`Building has been successfully removed`,
                    },
                },
            },
            withMutation: withBuildingMutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <BuildingUpsertForm
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            name: {
                render: ({item}) => <ButtonLink
                    icon={<BuildingIcon/>}
                    href={{
                        href:  "/manager/building/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                    label={item.name}
                />,
            },
        }}
        withQueryStore={BuildingQueryStore}
        withSourceQuery={withBuildingQuery}
        {...props}
    />;
};
