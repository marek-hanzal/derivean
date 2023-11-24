"use client";

import {
    BuildingComponents,
    BuildingConstructionRequirement,
    BuildingInline,
    BuildingRequirement
}          from "@derivean/building";
import {
    BuildingIcon,
    ConstructionIcon
}          from "@derivean/ui";
import {
    ButtonLink,
    HumanTime
}          from "@use-pico/client";
import {t} from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}          from "react";

export namespace KingdomConstructionTable {
    export type Columns =
        | "name"
        | "construction"
        | "maximum"
        | "time"
        | "requirements";

    export type Props =
        Omit<
            ComponentProps<typeof BuildingComponents.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            kingdomId: string;
        }
}

export const KingdomConstructionTable: FC<KingdomConstructionTable.Props> = (
    {
        kingdomId,
        ...props
    },
) => {
    return <BuildingComponents.Table
        text={{
            total: t`Total count of buildings`,
        }}
        icon={<ConstructionIcon/>}
        columns={{
            name:         {
                title: t`Building name`,
                render: ({item}) => <ButtonLink
                    icon={<BuildingIcon/>}
                    href={{
                        href:  "/kingdom/[kingdomId]/building/construction/available/[buildingId]",
                        query: {
                            kingdomId,
                            buildingId: item.id,
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
                width: "w-10",
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
                width: "w-10",
            },
        }}
        {...props}
    />;
};
