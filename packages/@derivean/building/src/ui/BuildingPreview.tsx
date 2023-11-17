import {
    Group,
    Preview,
    Text
}                            from "@use-pico/client";
import {type Infer}          from "@use-pico/extras";
import {t}                   from "@use-pico/translator";
import {type FC}             from "react";
import {BuildingInline}      from "../inline/BuildingInline";
import {BuildingSchema}      from "../schema/BuildingSchema";
import {BuildingRequirement} from "./BuildingRequirement";

export namespace BuildingPreview {
    export interface Props {
        building: Infer.Entity<BuildingSchema>;
    }
}

export const BuildingPreview: FC<BuildingPreview.Props> = (
    {
        building,
    }
) => {
    return <Preview
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
