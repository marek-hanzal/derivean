import {
    type FilterSchema,
    type OrderBySchema,
    QuerySchema
}                                  from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
}                                  from "@use-pico2/schema";
import {type IMultiSelectionStore} from "@use-pico2/selection";
import {MultiQueryInput}           from "../input/MultiQueryInput";
import type {ValuesSchema}         from "../schema/ValuesSchema";

export namespace withSourceMultiQueryInput {
    export interface Props<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TResponseSchema extends WithIdentitySchema,
    > {
        // withSourceQuery: WithSourceQuery<TQuerySchema, TResponseSchema>;
        MultiSelectionStore: IMultiSelectionStore<PicoSchema.Output<TResponseSchema>>;
    }
}

export const withSourceMultiQueryInput = <
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        // withSourceQuery,
        MultiSelectionStore,
    }: withSourceMultiQueryInput.Props<TQuerySchema, TResponseSchema>
) => {
    return function <
        TValuesSchema extends ValuesSchema,
    >(
        {
            // queryDefaults,
            ...props
        }: Omit<MultiQueryInput.Props<TValuesSchema, TResponseSchema, TQuerySchema>, "withSourceQuery" | "MultiSelectionStore" | "withFindByQuery"> & {
            // queryDefaults?: ComponentProps<WithSourceQuery<TResponseSchema, TQuerySchema>["store"]["Provider"]>["defaults"]
        }
    ) {
        return "withSourceMultiQueryInput";

        // return <withSourceQuery.store.Provider
        //     defaults={{
        //         cursor: {
        //             page: 0,
        //             size: 10,
        //         },
        //         ...queryDefaults,
        //     }}
        // >
        //     <MultiQueryInput
        //         withSourceQuery={withSourceQuery}
        //         MultiSelectionStore={MultiSelectionStore}
        //         {...props}
        //     />
        // </withSourceQuery.store.Provider>;
    };
};
