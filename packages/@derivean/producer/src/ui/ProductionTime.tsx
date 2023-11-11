"use client";

import {
    QueryResult,
    useQueryEx
}                                from "@use-pico/query";
import {type FC}                 from "react";
import {withProductionTimeQuery} from "../query/withProductionTimeQuery";

export namespace ProductionTime {
    export interface Props {
        producerId: string;
    }
}

export const ProductionTime: FC<ProductionTime.Props> = (
    {
        producerId,
    }
) => {
    const result = useQueryEx({
        withQuery: withProductionTimeQuery,
        request:   {
            id: producerId,
        }
    });

    return <QueryResult
        result={result}
        WithSuccess={({entity}) => {
            return <>
                <h1>not yet [ProductionTime]</h1>
            </>;
        }}
    />;
};
