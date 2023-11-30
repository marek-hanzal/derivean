import {
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
                key: "name",
                label: t`Producer name`,
                value: <div className={"flex gap-4"}>
                           <ProducerInline entity={entity}/>
                           <Text c={"dimmed"}>
                               ({entity.name})
                           </Text>
                       </div>,
            },
            {
                key: "time",
                label: t`Production time`,
                value: <HumanTime seconds={entity.time}/>,
            },
            {
                key: "pipeline",
                label: t`Producer pipeline time`,
                value: <ProductionTime producerId={entity.id}/>,
            },
            {
                key: "input",
                label: t`Producer input`,
                value: <ProducerInput producerId={entity.id}/>
            },
            {
                key: "output",
                label: t`Producer output`,
                value: <ProducerOutput producerId={entity.id}/>
            },
        ]}
    />;
};
