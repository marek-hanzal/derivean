"use client";

import {
    ResourceInline,
    ResourceUI
}                                      from "@derivean/resource";
import {
    BuildingIcon,
    ResourceIcon
}                                      from "@derivean/ui";
import {t}                             from "@use-pico/i18n";
import {
    ButtonLink,
    Loader
}                                      from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                                      from "react";
import {BuildingRequirementUpsertForm} from "../form/BuildingRequirementUpsertForm";
import {BuildingInline}                from "../inline/BuildingInline";
import {BuildingRequirementRpc}        from "../rpc/BuildingRequirementRpc";
import {BuildingRequirementUI}         from "../ui/BuildingRequirementUI";
import {BuildingUI}                    from "../ui/BuildingUI";

export namespace BuildingRequirementTable {
    export type Columns =
        | "building"
        | "resource"
        | "amount";

    export type Props =
        Omit<
            ComponentProps<typeof BuildingRequirementUI.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            buildingId?: string;
        }
}

export const BuildingRequirementTable: FC<BuildingRequirementTable.Props> = (
    {
        buildingId,
        ...props
    }
) => {
    return <BuildingRequirementUI.Table
        text={{
            total: t()`Total count of building requirements`,
        }}
        name={"building-requirement"}
        icon={<ResourceIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create building requirement (title)`,
                    label: t()`Create building requirement`,
                },
            },
            upsertForm: ({modalId}) => <BuildingRequirementUpsertForm
                buildingId={buildingId}
                withAutoClose={[modalId]}
            />,
        }}
        rowActionProps={{
            text:         {
                update: {
                    title: t()`Update building requirement`,
                    label: t()`Update building requirement`,
                },
                delete: {
                    label: t()`Delete building requirement`,
                    modal: {
                        title:   t()`Delete building requirement (modal)`,
                        content: t()`Do you really want to delete selected building requirement?`,
                        success: {
                            title:   t()`Success`,
                            message: t()`Building requirement has been successfully removed`,
                        },
                    },
                },
            },
            withMutation: BuildingRequirementRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <BuildingRequirementUpsertForm
                buildingId={buildingId}
                withAutoClose={[modalId]}
                entity={item}
            />,
        }}
        columns={{
            building: {
                title:  t()`Building name`,
                render: ({item}) => <BuildingUI.Fetch
                    override={item.buildingId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<BuildingIcon/>}
                        href={{
                            href:  "/manager/building/[id]",
                            query: {
                                id: entity.id,
                            },
                        }}
                        label={<BuildingInline entity={entity}/>}
                    />}
                />,
                width:  18,
            },
            resource: {
                title:  t()`Resource name`,
                render: ({item}) => <ResourceUI.Fetch
                    override={item.resourceId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<ResourceIcon/>}
                        href={{
                            href:  "/manager/resource/[id]",
                            query: {
                                id: entity.id,
                            },
                        }}
                        label={<ResourceInline entity={entity}/>}
                    />}
                />,
            },
            amount:   {
                title:  t()`Required amount`,
                render: ({item}) => item.amount,
                width:  18,
            },
        }}
        {...props}
    />;
};
