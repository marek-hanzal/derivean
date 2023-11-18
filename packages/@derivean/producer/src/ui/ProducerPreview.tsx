import {
    Group,
    HumanTime,
    Preview,
    Text,
    type WithEntity
}                       from "@use-pico/client";
import {type Infer}     from "@use-pico/extras";
import {t}              from "@use-pico/translator";
import {type FC}        from "react";
import {ProducerInline} from "../inline/ProducerInline";
import {ProducerSchema} from "../schema/ProducerSchema";
import {ProducerInput}  from "./ProducerInput";
import {ProducerOutput} from "./ProducerOutput";
import {ProductionTime} from "./ProductionTime";

export namespace ProducerPreview {
    export interface Props extends WithEntity<Infer.Entity<ProducerSchema>> {
    }
}

export const ProducerPreview: FC<ProducerPreview.Props> = (
    {
        entity,
    }
) => {
    return <Preview
        items={[
            {
                label: t()`Producer name`,
                value: <Group gap={"xs"}>
                           <ProducerInline entity={entity}/>
                           <Text c={"dimmed"}>
                               ({entity.name})
                           </Text>
                       </Group>,
            },
            {
                label: t()`Production time`,
                value: <HumanTime seconds={entity.time}/>,
            },
            {
                label: t()`Producer pipeline time`,
                value: <ProductionTime producerId={entity.id}/>,
            },
            {
                label: t()`Producer input`,
                value: <ProducerInput producerId={entity.id}/>
            },
            {
                label: t()`Producer output`,
                value: <ProducerOutput producerId={entity.id}/>
            },
        ]}
    />;
};
