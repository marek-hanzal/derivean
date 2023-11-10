import {
    type FilterSchema,
    type IQueryStore,
    type OrderBySchema,
    type QuerySchema
}                                  from "@use-pico/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                                  from "@use-pico/schema";
import {type IMultiSelectionStore} from "@use-pico/selection";
import {type IWithSourceQuery}     from "@use-pico/source";
import {StoreProvider}             from "@use-pico/store";
import {MultiQueryInput}           from "../input/MultiQueryInput";
import type {ValuesSchema}         from "../schema/ValuesSchema";

export namespace withSourceMultiQueryInput {
    export interface Props<
        TResponseSchema extends WithIdentitySchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > {
        withQueryStore: IQueryStore.Store<TQuerySchema>;
        withSourceQuery: IWithSourceQuery<TQuerySchema, TResponseSchema>;
        MultiSelectionStore: IMultiSelectionStore<PicoSchema.Output<TResponseSchema>>;
    }
}

export const withSourceMultiQueryInput = <
    TResponseSchema extends WithIdentitySchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
>(
    {
        withQueryStore,
        withSourceQuery,
        MultiSelectionStore,
    }: withSourceMultiQueryInput.Props<
        TResponseSchema,
        TQuerySchema
    >
) => {
    return function <
        TValuesSchema extends ValuesSchema,
    >(
        {
            queryDefaults,
            ...props
        }: Omit<
            MultiQueryInput.Props<
                TValuesSchema,
                TResponseSchema,
                TQuerySchema
            >,
            "withSourceQuery" | "MultiSelectionStore" | "withFindByQuery"
        > & {
               queryDefaults?: PicoSchema.Output<TQuerySchema>;
           }
    ) {
        return <StoreProvider
            store={withQueryStore}
            values={{
                cursor: {
                    page: 0,
                    size: 10,
                },
                ...queryDefaults,
            }}
        >
            <MultiQueryInput
                withSourceQuery={withSourceQuery}
                MultiSelectionStore={MultiSelectionStore}
                {...props}
            />
        </StoreProvider>;
    };
};
