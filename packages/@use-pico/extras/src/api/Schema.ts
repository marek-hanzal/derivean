import {
    type FilterSchema,
    type MutationSchema,
    type OrderBySchema,
    type QuerySchema
} from "@use-pico/query";
import {
    type ObjectSchema,
    type WithIdentitySchema
} from "@use-pico/schema";

/**
 * This is all-in-one schema used across the library.
 */
export interface Schema<
    TEntity extends WithIdentitySchema,
    TShapeSchema extends ObjectSchema<any>,
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQueryOutput extends QuerySchema<TFilterSchema, TOrderBySchema> = QuerySchema<
        TFilterSchema,
        TOrderBySchema
    >,
    TMutationOutput extends MutationSchema<TShapeSchema, TQueryOutput> = MutationSchema<
        TShapeSchema,
        TQueryOutput
    >,
> {
    entity: TEntity;
    shape: TShapeSchema;
    filter: TFilterSchema;
    orderBy: TOrderBySchema;
    query: TQueryOutput;
    mutation: TMutationOutput;
}
