import {type PicoSchema} from "@use-pico/schema";
import {type Schema}     from "./Schema";

/**
 * Tool to infer any schema from the library.
 */
export namespace Infer {
    export type Entity<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["entity"]>;

    export type Entity$<
        TSchema extends Schema<any, any, any, any>
    > = Partial<PicoSchema.Output<TSchema["entity"]>>;

    export type EntityWithoutId<
        TSchema extends Schema<any, any, any, any>
    > = Omit<PicoSchema.Output<TSchema["entity"]>, "id">;

    export type EntitySchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["entity"];

    export type Shape<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["shape"]>;

    export type ShapeSchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["shape"];

    export type Filter<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["filter"]>

    export type FilterSchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["filter"];

    export type OrderBy<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["orderBy"]>;

    export type OrderBySchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["orderBy"];

    export type Query<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["query"]>;

    export type QuerySchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["query"];

    export type Mutation<
        TSchema extends Schema<any, any, any, any>
    > = PicoSchema.Output<TSchema["mutation"]>;

    export type MutationSchema<
        TSchema extends Schema<any, any, any, any>
    > = TSchema["mutation"];

    export type Create<
        TSchema extends Schema<any, any, any, any>
    > = NonNullable<Mutation<TSchema>["create"]>;

    export type Update<
        TSchema extends Schema<any, any, any, any>
    > = NonNullable<Mutation<TSchema>["update"]>;

    export type Upsert<
        TSchema extends Schema<any, any, any, any>
    > = NonNullable<Mutation<TSchema>["upsert"]>;

    export type Delete<
        TSchema extends Schema<any, any, any, any>
    > = NonNullable<Mutation<TSchema>["delete"]>;
}
