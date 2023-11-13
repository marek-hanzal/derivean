import {type Database}      from "@use-pico/orm";
import {
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                           from "@use-pico/query";
import {
    PicoSchema,
    type WithIdentitySchema
}                           from "@use-pico/schema";
import {
    type MutationSchema,
    type ShapeSchema
}                           from "@use-pico/source";
import {type IWithApply}    from "./IWithApply";
import {type IWithMutation} from "./IWithMutation";
import {type IWithQuery}    from "./IWithQuery";

export interface IRepository<
    TDatabase extends Database,
    TSchema extends IRepository.Schema<
        any,
        any,
        QuerySchema<FilterSchema, OrderBySchema>,
        MutationSchema<ShapeSchema, QuerySchema<FilterSchema, OrderBySchema>>
    >,
    TTable extends keyof TDatabase & string,
> extends IWithApply<
    TDatabase,
    TSchema,
    TTable
> {
    get withQuery(): IWithQuery<TDatabase, TSchema, TTable>;

    get withMutation(): IWithMutation<TDatabase, TSchema, TTable>;

    get(id: string): Promise<PicoSchema.Output<TSchema["entity"]> | undefined>;

    getOrThrow(id: string): Promise<PicoSchema.Output<TSchema["entity"]>>;

    toCreate(create: NonNullable<PicoSchema.Output<TSchema["mutation"]["shape"]["create"]>>): Promise<Omit<PicoSchema.Output<TSchema["entity"]>, "id">>;

    toUpdate(update: NonNullable<PicoSchema.Output<TSchema["mutation"]["shape"]["update"]>>["update"]): Promise<Partial<PicoSchema.Output<TSchema["entity"]>>>;
}

export namespace IRepository {
    export interface Schema<
        TEntitySchema extends WithIdentitySchema,
        TShapeSchema extends ShapeSchema,
        TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
        TMutationSchema extends MutationSchema<TShapeSchema, TQuerySchema>,
    > {
        readonly entity: TEntitySchema;
        readonly shape: TShapeSchema;
        readonly query: TQuerySchema;
        readonly mutation: TMutationSchema;
    }
}
