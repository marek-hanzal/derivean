"use client";

import {BuildingIcon}         from "@derivean/ui";
import {withDullSchema}       from "@use-pico/dull-stuff";
import {t}                    from "@use-pico/i18n";
import {ButtonLink}           from "@use-pico/ui";
import {Table}                from "@use-pico/ui-extra";
import {type FC}              from "react";
import {BuildingUpsertForm}   from "../form/BuildingUpsertForm";
import {withBuildingMutation} from "../mutation/withBuildingMutation";
import {BuildingQueryStore}   from "../query/BuildingQueryStore";
import {withBuildingQuery}    from "../query/withBuildingQuery";
import {type BuildingSchema}  from "../schema/BuildingSchema";

export namespace BuildingTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        Table.Props<
            Columns,
            withDullSchema.Infer.EntitySchema<BuildingSchema>,
            withDullSchema.Infer.QuerySchema<BuildingSchema>
        >,
        "columns" | "withSourceQuery" | "withQueryStore" | "name" | "icon" | "text"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <Table
        text={{
            total: t()`Total count of buildings`,
        }}
        name={"building"}
        icon={<BuildingIcon/>}
        tableActionProps={{
            text: {
                create: {
                    title: t()`Create new building`,
                    label: t()`Create building`,
                },
            },
            upsertForm: ({modalId}) => <BuildingUpsertForm
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text: {
                update: {
                    title: t()`Update building`,
                    label: t()`Update building`,
                },
                delete: {
                    label: t()`Delete building`,
                    modal: {
                        title:   t()`Delete building (modal)`,
                        content: t()`Do you really want to delete selected building?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Building has been successfully removed`,
                        },
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
                title: t()`Building name`,
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
