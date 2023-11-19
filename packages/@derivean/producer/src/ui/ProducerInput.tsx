import {
    ItemFetch,
    ItemInline
}                                from "@derivean/item";
import {
    Nav,
    Text
}                                from "@use-pico/client";
import {t}                       from "@use-pico/translator";
import {type FC}                 from "react";
import {ProducerInputCollection} from "./ProducerInputComponents";

export namespace ProducerInput {
    export interface Props {
        mark?: string[];
        producerId: string;
    }
}

export const ProducerInput: FC<ProducerInput.Props> = (
    {
        mark = [],
        producerId,
    }
) => {
    return <ProducerInputCollection
        query={{
            where: {
                producerId,
            }
        }}
        WithSuccess={({entities}) => <>
            {entities.length > 0 && <Nav
                separator={"&"}
                separatorMargin={"gap-4"}
                items={entities.map(entity => ({
                    type:      "custom",
                    component: <ItemFetch
                                   key={entity.id}
                                   override={entity.itemId}
                                   WithSuccess={({entity}) => <Text
                                       fw={mark.includes(entity.id) ? 600 : undefined}
                                       c={mark.includes(entity.id) ? undefined : "dimmed"}
                                   >
                                       <ItemInline entity={entity}/>
                                   </Text>}
                               />,
                }))}
            />}
            {!entities.length && <Text
                c={"dimmed"}
            >
                {t()`Producer without inputs`}
            </Text>}
        </>}
    />;
};
