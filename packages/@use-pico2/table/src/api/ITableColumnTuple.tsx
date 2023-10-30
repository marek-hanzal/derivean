import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                          from "@use-pico2/query";
import {type PicoSchema}   from "@use-pico2/schema";
import {type ITableColumn} from "./ITableColumn";

export type ITableColumnTuple<
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TSchema extends PicoSchema,
> = [string, ITableColumn<TQuerySchema, TSchema>];
