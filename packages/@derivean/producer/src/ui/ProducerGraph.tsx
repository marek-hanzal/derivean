"use client";

import {
    BlockStore,
    Diagram,
    QueryResult,
    Skeleton,
    useQueryEx,
    useWithLocaleRedirect
}                  from "@use-pico/client";
import {type FC}   from "react";
import {withGraph} from "../query/withGraph";

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
    const block = BlockStore.useSelector(({block}) => ({block}));
    const result = useQueryEx({
        withQuery: withGraph,
        request:   {
            id: producerId,
        }
    });

    return <QueryResult
        result={result}
        WithLoading={() => <Skeleton lines={12}/>}
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
