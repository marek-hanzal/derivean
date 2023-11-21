"use client";

import {BuildingIcon}                    from "@derivean/ui";
import {
    ButtonLink,
    HumanTime
}                                        from "@use-pico/client";
import {t}                               from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                                        from "react";
import {BuildingInline}                  from "../inline/BuildingInline";
import {BuildingComponents}              from "../ui/BuildingComponents";
import {BuildingConstructionRequirement} from "../ui/BuildingConstructionRequirement";
import {BuildingRequirement}             from "../ui/BuildingRequirement";

export namespace BuildingTable {
    export type Columns =
        | "name"
        | "construction"
        | "maximum"
        | "time"
        | "requirements";

    export type Props = Omit<
        ComponentProps<typeof BuildingComponents.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const BuildingTable: FC<BuildingTable.Props> = props => {
    return <BuildingComponents.Table
        text={{
            total: t`Total count of buildings`,
        }}
        icon={<BuildingIcon/>}
        // tableActionProps={{
        //     text: {
        //         create: {
        //             title: t()`Create new building`,
        //             label: t()`Create building`,
        //         },
        //     },
        //     upsertForm: ({modalId}) => <BuildingUpsertForm
        //         withAutoClose={[modalId]}
        //     />,
        // }}
        // rowActionProps={{
        //     text:         {
        //         update: {
        //             title: t()`Update building`,
        //             label: t()`Update building`,
        //         },
        //         delete: {
        //             label: t()`Delete building`,
        //             modal: {
        //                 title:   t()`Delete building (modal)`,
        //                 content: t()`Do you really want to delete selected building?`,
        //                 success: {
        //                     title:   t()`Success`,
        //                     message: t()`Building has been successfully removed`,
        //                 },
        //             },
        //         },
        //     },
        //     withMutation: BuildingRpc.mutation,
        //     upsertForm:   ({
        //                        item,
        //                        modalId
        //                    }) => <BuildingUpsertForm
        //         withAutoClose={[modalId]}
        //         entity={item}
        //     />,
        // }}
        columns={{
            name:         {
                title: t`Building name`,
                render: ({item}) => <ButtonLink
                    icon={<BuildingIcon/>}
                    href={{
                        href:  "/manager/building/[id]",
                        query: {
                            id: item.id,
                        },
                    }}
                >
                    <BuildingInline entity={item}/>
                </ButtonLink>,
            },
            construction: {
                title: t`Building construction requirement (label)`,
                render: ({item}) => <BuildingConstructionRequirement
                    buildingId={item.id}
                />,
            },
            time:         {
                title: t`Building construction time (label)`,
                render: ({item}) => <HumanTime seconds={item.construction}/>,
                width:  10,
            },
            requirements: {
                title: t`Building requirement (label)`,
                render: ({item}) => <BuildingRequirement
                    buildingId={item.id}
                />,
            },
            maximum:      {
                title: t`Building construction limit (label)`,
                render: ({item}) => item.maximum,
                width:  10,
            },
        }}
        {...props}
    />;
};
