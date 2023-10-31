"use client";

import {
    QueryResult,
    useQueryEx
}                                   from "@use-pico/query";
import {
    type FC,
    type PropsWithChildren
}                                   from "react";
import {type IWithTranslationQuery} from "../api/IWithTranslationQuery";
import {type TranslationSchema}     from "../schema/TranslationSchema";

export namespace TranslationProvider {
    export type Props = PropsWithChildren<{
        locale: string;
        withTranslationQuery: IWithTranslationQuery;
        loading?: QueryResult.Props<TranslationSchema>["WithLoading"];
    }>
}

export const TranslationProvider: FC<TranslationProvider.Props> = (
    {
        locale,
        withTranslationQuery,
        children,
    }
) => {
    const result = useQueryEx({
        withQuery: withTranslationQuery,
        request:   {
            locale,
        },
    });

    return <QueryResult
        result={result}
        WithSuccess={({data}) => children}
        WithError={() => children}
    />;
};
