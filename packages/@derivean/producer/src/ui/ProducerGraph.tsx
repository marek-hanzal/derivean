"use client";

import {Diagram}               from "@use-pico/diagram";
import {useWithLocaleRedirect} from "@use-pico/i18n";
import {
    QueryResult,
    useQueryEx
}                              from "@use-pico/query";
import {useStore}              from "@use-pico/store";
import {
    BlockStore,
    SkeletonBlock
}                              from "@use-pico/ui";
import {type FC}               from "react";
import {withGraph}             from "../query/withGraph";

export namespace ProducerGraph {
    export interface Props extends Omit<Diagram.Props, "graph"> {
        producerId: string;
    }
}

export const ProducerGraph: FC<ProducerGraph.Props> = (
    {
        producerId,
        ...props
    }
) => {
    const redirect = useWithLocaleRedirect();
    const block = useStore(BlockStore, ({block}) => ({block}));
    const result = useQueryEx({
        withQuery: withGraph,
        request:   {
            id: producerId,
        }
    });

    return <QueryResult
        result={result}
        WithLoading={() => <SkeletonBlock lines={12}/>}
        WithSuccess={({entity}) => <Diagram
            graph={entity}
            onClick={id => {
                block.block();
                redirect({
                    href:  `/manager/producer/[id]/pipeline`,
                    query: {
                        id,
                    },
                });
            }}
            {...props}
        />}
    />;
};
