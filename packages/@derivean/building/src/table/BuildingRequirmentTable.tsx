"use client";

import {
    ItemInline,
    ItemUI
}                                      from "@derivean/item";
import {
    BuildingIcon,
    ItemIcon
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
import {BuildingRequirementComponents} from "../ui/BuildingRequirementComponents";
import {BuildingUI}                    from "../ui/BuildingUI";

export namespace BuildingRequirementTable {
    export type Columns =
        | "building"
        | "item"
        | "amount";

    export type Props =
        Omit<
            ComponentProps<typeof BuildingRequirementComponents.Table<Columns>>,
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
    return <BuildingRequirementComponents.Table
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
            item: {
                title:  t()`Item name`,
                render: ({item}) => <ItemUI.Fetch
                    override={item.itemId}
                    loader={<Loader size={"md"} type={"dots"}/>}
                    WithSuccess={({entity}) => <ButtonLink
                        icon={<ItemIcon/>}
                        href={{
                            href: "/manager/item/[id]",
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
