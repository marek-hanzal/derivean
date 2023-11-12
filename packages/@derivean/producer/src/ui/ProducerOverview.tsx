"use client";

import {ProducerIcon}       from "@derivean/ui";
import {List}               from "@use-pico/table";
import {
    ButtonLink,
    Preview
}                           from "@use-pico/ui";
import {type FC}            from "react";
import {ProducerInline}     from "../inline/ProducerInline";
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
                    label: <ButtonLink
                               icon={<ProducerIcon/>}
                               href={{
                                   href:  `/manager/producer/[id]/pipeline`,
                                   query: {
                                       id: item.id,
                                   },
                               }}
                               label={<ProducerInline entity={item}/>}
                           />,
                    value: <ProducerGraph
                               zoom={false}
                               producerId={item.id}
                           />,
                }
            ]}
        />}
    />;
};
