import {
    type CursorSchema,
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                        from "@use-pico/query";
import {type PicoSchema} from "@use-pico/schema";
import {type IStore}     from "./IStore";

export type IQueryStore<
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>
> = IStore<{
    schema: TQuerySchema;

    hasWhere(): boolean;
    hasFilter(): boolean;

    setCursor(page: number, size?: number): void;
    setSize(size: number): void;

    setFilter(filter: PicoSchema.Output<TQuerySchema["shape"]["filter"]>): void;
    shallowFilter(filter?: PicoSchema.Output<TQuerySchema["shape"]["filter"]>): void;
    clearFilter(): void;
    isFilter(): boolean;

    setOrderBy(orderBy: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>): void;
    shallowOrderBy(orderBy?: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>): void;
}, {
    /**
     * If set, mandatory filters which user cannot change.
     *
     * Can be set only when a store is created (instance of store).
     */
    where?: PicoSchema.Output<TQuerySchema["shape"]["filter"]>;
    /**
     * User-specific filters; "where" takes precedence.
     */
    filter?: PicoSchema.Output<TQuerySchema["shape"]["filter"]>;
    orderBy?: PicoSchema.Output<TQuerySchema["shape"]["orderBy"]>;
    cursor?: CursorSchema.Type | null;
}>;

export namespace IQueryStore {
    export type Store<
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    > = IStore.Store<
        IQueryStore<TQuerySchema>
    >;
}
