import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                            from "@use-pico2/query";
import {type ObjectSchema}   from "@use-pico2/schema";
import {type MutationSchema} from "./MutationSchema";
import {type ShapeSchema}    from "./ShapeSchema";

export interface SourceSchema<
    TEntitySchema extends ObjectSchema<any>,
    TShapeSchema extends ShapeSchema,
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
    TQuerySchema extends QuerySchema<TFilterSchema, TOrderBySchema> = QuerySchema<TFilterSchema, TOrderBySchema>
> {
    entity: TEntitySchema;
    shape: TShapeSchema;
    mutation: MutationSchema<TShapeSchema, TQuerySchema>;
    query: TQuerySchema;
    filter: TFilterSchema;
    orderBy: TOrderBySchema;
}
