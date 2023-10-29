import {type PrismaClient} from "@use-pico2/orm";
import {
    type CountSchema,
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                          from "@use-pico2/query";
import {type IRepository}  from "@use-pico2/repository";
import {
    type PicoSchema,
    type WithIdentitySchema
}                          from "@use-pico2/schema";
import {
    type MutationSchema,
    type ShapeSchema
}                          from "@use-pico2/source";

export abstract class AbstractRepository<
    TEntitySchema extends WithIdentitySchema,
    TShapeSchema extends ShapeSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TMutationSchema extends MutationSchema<TShapeSchema, TQuerySchema>,
> implements IRepository<
    TEntitySchema,
    TShapeSchema,
    TQuerySchema,
    TMutationSchema
> {
    protected constructor(
        readonly entitySchema: TEntitySchema,
        readonly shapeSchema: TShapeSchema,
        readonly querySchema: TQuerySchema,
        readonly mutationSchema: TMutationSchema,
        readonly prisma: PrismaClient,
    ) {
    }

    abstract count(query: PicoSchema.Output<TQuerySchema>): Promise<CountSchema.Type>;
}
