import {type IStoreSchema}     from "@use-pico2/store";
import {type FilterSchema}     from "../schema/FilterSchema";
import {type OrderBySchema}    from "../schema/OrderBySchema";
import {type QuerySchema}      from "../schema/QuerySchema";
import {type IQueryStoreProps} from "./IQueryStoreProps";

export type IQueryStore<
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
> =
    IStoreSchema<
        IQueryStoreProps<TQuerySchema>
    >["Store"];
