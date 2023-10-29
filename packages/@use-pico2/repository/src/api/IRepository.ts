import {
    type CountSchema,
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
} from "@use-pico2/query";
import {
    type PicoSchema,
    type WithIdentitySchema
} from "@use-pico2/schema";
import {
    type MutationSchema,
    type ShapeSchema
} from "@use-pico2/source";

export interface IRepository<
    TEntitySchema extends WithIdentitySchema,
    TShapeSchema extends ShapeSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TMutationSchema extends MutationSchema<TShapeSchema, TQuerySchema>,
> {
    readonly entitySchema: TEntitySchema;
    readonly shapeSchema: TShapeSchema;
    readonly querySchema: TQuerySchema;
    readonly mutationSchema: TMutationSchema;

    count(query: PicoSchema.Output<TQuerySchema>): Promise<CountSchema.Type>;
}
