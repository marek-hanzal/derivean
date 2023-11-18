"use client";

import {
    HumanTime,
    Loader,
    QueryResult,
    useQueryEx
}                                from "@use-pico/client";
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
        WithLoading={() => <Loader/>}
        WithSuccess={({entity}) => entity.time ? <HumanTime seconds={entity.time}/> : "-"}
    />;
};
