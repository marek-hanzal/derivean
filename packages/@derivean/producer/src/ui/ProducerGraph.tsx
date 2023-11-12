"use client";

import {Diagram}       from "@use-pico/diagram";
import {
    QueryResult,
    useQueryEx
}                      from "@use-pico/query";
import {SkeletonBlock} from "@use-pico/ui";
import {type FC}       from "react";
import {withGraph}     from "../query/withGraph";

export namespace ProducerGraph {
    export interface Props {
        producerId: string;
    }
}

export const ProducerGraph: FC<ProducerGraph.Props> = (
    {
        producerId,
    }
) => {
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
        />}
    />;
};
