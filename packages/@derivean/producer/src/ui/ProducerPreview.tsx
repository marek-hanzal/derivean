import {withDullSchema} from "@use-pico/dull-stuff";
import {t}              from "@use-pico/i18n";
import {Preview}        from "@use-pico/ui";
import {HumanSeconds}   from "@use-pico/ui-extra";
import {type FC}        from "react";
import {ProducerSchema} from "../schema/ProducerSchema";
import {ProducerInput}  from "./ProducerInput";
import {ProducerOutput} from "./ProducerOutput";
import {ProductionTime} from "./ProductionTime";

export namespace ProducerPreview {
    export interface Props {
        producer: withDullSchema.Infer.Entity<ProducerSchema>;
    }
}

export const ProducerPreview: FC<ProducerPreview.Props> = (
    {
        producer,
    }
) => {
    return <Preview
        cols={4}
        items={[
            {
                label: t()`Producer name`,
                value: producer.name,
            },
            {
                label: t()`Production time`,
                value: <HumanSeconds seconds={producer.time}/>,
            },
            {
                label: t()`Producer pipeline time`,
                value: <ProductionTime producerId={producer.id}/>,
            },
            {
                label: t()`Producer input`,
                value: <ProducerInput producerId={producer.id}/>
            },
            {
                label: t()`Producer output`,
                value: <ProducerOutput producerId={producer.id}/>
            },
        ]}
    />;
};
