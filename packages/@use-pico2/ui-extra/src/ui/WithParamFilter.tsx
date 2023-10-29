"use client";

import {useParam$}         from "@use-pico2/navigation";
import {type FilterSchema} from "@use-pico2/query";
import {
    parseJson$,
    type PicoSchema
}                          from "@use-pico2/schema";
import {type ReactNode}    from "react";

export namespace WithParamFilter {
    export interface Props<
        TFilterSchema extends FilterSchema,
    > {
        name?: string;
        schema: TFilterSchema;

        children(filter?: PicoSchema.Output<TFilterSchema>): ReactNode;
    }
}

export const WithParamFilter = <
    TFilterSchema extends FilterSchema,
>(
    {
        name = "filter",
        schema,
        children,
    }: WithParamFilter.Props<TFilterSchema>
) => {
    return children(
        parseJson$(schema, useParam$(name))
    );
};
