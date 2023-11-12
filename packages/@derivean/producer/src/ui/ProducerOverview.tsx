"use client";

import {List}               from "@use-pico/table";
import {Preview}            from "@use-pico/ui";
import {type FC}            from "react";
import {ProducerRpc}        from "../rpc/ProducerRpc";
import {ProducerQueryStore} from "../store/ProducerQueryStore";
import {ProducerGraph}      from "./ProducerGraph";

export namespace ProducerOverview {
    export interface Props {
    }
}

export const ProducerOverview: FC<ProducerOverview.Props> = () => {
    return <List
        withQueryStore={ProducerQueryStore}
        withSourceQuery={ProducerRpc.query}
        Item={({item}) => <Preview
            key={item.id}
            cols={1}
            items={[
                {
                    label: item.name,
                    value: <ProducerGraph
                               zoom={false}
                               producerId={item.id}
                           />,
                }
            ]}
        />}
    />;
};
