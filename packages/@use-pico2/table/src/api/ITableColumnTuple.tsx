import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                          from "@use-pico2/query";
import {type PicoSchema}   from "@use-pico2/schema";
import {type ITableColumn} from "./ITableColumn";

export type ITableColumnTuple<
    TSchema extends PicoSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
> = [string, ITableColumn<TSchema, TQuerySchema>];
