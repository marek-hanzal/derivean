"use client";

import {
    ProducerInput,
    ProducerOutput,
    ProducerUI,
    ProductionTime
}                            from "@derivean/producer";
import {withDullSchema}      from "@use-pico/dull-stuff";
import {t}                   from "@use-pico/i18n";
import {
    Group,
    Loader,
    Preview,
    Text
}                            from "@use-pico/ui";
import {HumanSeconds}        from "@use-pico/ui-extra";
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
                label: t()`Production time`,
                value: <ProducerUI.Fetch
                           override={building.producerId}
                           loader={<Loader size={"md"} type={"dots"}/>}
                           WithSuccess={({entity}) => <HumanSeconds seconds={entity.time}/>}
                       />,
            },
            {
                label: t()`Building pipeline time`,
                value: <ProductionTime producerId={building.producerId}/>,
            },
            {
                label: t()`Building requirement (label)`,
                value: <BuildingRequirement buildingId={building.id}/>
            },
            {
                label: t()`Building input`,
                value: <ProducerInput producerId={building.producerId}/>
            },
            {
                label: t()`Building output`,
                value: <ProducerOutput producerId={building.producerId}/>
            },
        ]}
    />;
};
