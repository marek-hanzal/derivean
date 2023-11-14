"use client";

import {
    BuildingConstructionRequirement,
    BuildingInline,
    BuildingRequirement,
    BuildingUI
}                         from "@derivean/building";
import {ConstructionIcon} from "@derivean/ui";
import {t}                from "@use-pico/i18n";
import {HumanSeconds}     from "@use-pico/ui-extra";
import {
    type ComponentProps,
    type FC
}                         from "react";

export namespace KingdomConstructionTable {
    export type Columns =
        | "name"
        | "construction"
        | "maximum"
        | "time"
        | "requirements";

    export type Props = Omit<
        ComponentProps<typeof BuildingUI.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const KingdomConstructionTable: FC<KingdomConstructionTable.Props> = props => {
    return <BuildingUI.Table
        text={{
            total: t()`Total count of buildings`,
        }}
        name={"building"}
        icon={<ConstructionIcon/>}
        columns={{
            name:         {
                title:  t()`Building name`,
                render: ({item}) => <BuildingInline entity={item}/>,
            },
            construction: {
                title:  t()`Building construction requirement (label)`,
                render: ({item}) => <BuildingConstructionRequirement
                    buildingId={item.id}
                />,
            },
            time:         {
                title:  t()`Building construction time (label)`,
                render: ({item}) => <HumanSeconds seconds={item.construction}/>,
                width:  10,
            },
            requirements: {
                title:  t()`Building requirement (label)`,
                render: ({item}) => <BuildingRequirement
                    buildingId={item.id}
                />,
            },
            maximum:      {
                title:  t()`Building construction limit (label)`,
                render: ({item}) => item.maximum,
                width:  10,
            },
        }}
        {...props}
    />;
};
