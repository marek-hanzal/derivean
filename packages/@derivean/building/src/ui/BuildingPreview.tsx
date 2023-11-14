"use client";

import {withDullSchema}      from "@use-pico/dull-stuff";
import {t}                   from "@use-pico/i18n";
import {
    Group,
    Preview,
    Text
}                            from "@use-pico/ui";
import {type FC}             from "react";
import {BuildingInline}      from "../inline/BuildingInline";
import {BuildingSchema}      from "../schema/BuildingSchema";
import {BuildingRequirement} from "./BuildingRequirement";

export namespace BuildingPreview {
    export interface Props {
        building: withDullSchema.Infer.Entity<BuildingSchema>;
    }
}

export const BuildingPreview: FC<BuildingPreview.Props> = (
    {
        building,
    }
) => {
    return <Preview
        cols={3}
        items={[
            {
                label: t()`Building name`,
                value: <Group gap={"xs"}>
                           <BuildingInline entity={building}/>
                           <Text c={"dimmed"}>
                               ({building.name})
                           </Text>
                       </Group>,
            },
            {
                label: t()`Building requirement (label)`,
                value: <BuildingRequirement buildingId={building.id}/>
            },
        ]}
    />;
};
