"use client";

import {
    ItemInline,
    ItemUI
}                                                  from "@derivean/item";
import {
    BuildingIcon,
    ItemIcon
}                                                  from "@derivean/ui";
import {t}                                         from "@use-pico/i18n";
import {
    ButtonLink,
    Loader
}                                                  from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                                                  from "react";
import {BuildingConstructionRequirementUpsertForm} from "../form/BuildingConstructionRequirementUpsertForm";
import {BuildingInline}                            from "../inline/BuildingInline";
import {BuildingConstructionRequirementRpc}        from "../rpc/BuildingConstructionRequirementRpc";
import {BuildingConstructionRequirementUI}         from "../ui/BuildingConstructionRequirementUI";
import {BuildingUI}                                from "../ui/BuildingUI";

export namespace BuildingConstructionRequirementTable {
    export type Columns =
        | "building"
        | "item"
        | "amount";

    export type Props =
        Omit<
            ComponentProps<typeof BuildingConstructionRequirementUI.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            buildingId?: string;
        }
}

export const BuildingConstructionRequirementTable: FC<BuildingConstructionRequirementTable.Props> = (
    {
        buildingId,
        ...props
    }
) => {
    return <BuildingConstructionRequirementUI.Table
        text={{
            total: t()`Total count of building requirements`,
        }}
        name={"building-requirement"}
        icon={<ItemIcon/>}
        tableActionProps={{
            text:       {
                create: {
                    title: t()`Create building requirement (title)`,
                    label: t()`Create building requirement`,
                },
            },
            upsertForm: ({modalId}) => <BuildingConstructionRequirementUpsertForm
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
            withMutation: BuildingConstructionRequirementRpc.mutation,
            upsertForm:   ({
                               item,
                               modalId
                           }) => <BuildingConstructionRequirementUpsertForm
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
            item:     {
                title:  t()`Item name`,
                render: ({item}) => <ItemUI.Fetch
                    override={item.itemId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<ItemIcon/>}
                        href={{
                            href:  "/manager/item/[id]",
                            query: {
                                id: entity.id,
                            },
                        }}
                        label={<ItemInline entity={entity}/>}
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
