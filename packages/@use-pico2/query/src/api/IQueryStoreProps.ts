import {type PicoSchema}    from "@use-pico2/schema";
import {type IStoreProps}   from "@use-pico2/store";
import {type CursorSchema}  from "../schema/CursorSchema";
import {type FilterSchema}  from "../schema/FilterSchema";
import {type OrderBySchema} from "../schema/OrderBySchema";
import {type QuerySchema}   from "../schema/QuerySchema";

export type IQueryStoreProps<
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>
> = IStoreProps<{
    schema: TQuerySchema;
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
    cursor: CursorSchema.Type;

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
}>;
