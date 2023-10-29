import {type QuerySchema}        from "@use-pico2/query";
import {type WithIdentitySchema} from "@use-pico2/schema";

export type IWithFetchQuery<
    TQuerySchema extends QuerySchema<any, any>,
    TResponseSchema extends WithIdentitySchema,
> = {};
