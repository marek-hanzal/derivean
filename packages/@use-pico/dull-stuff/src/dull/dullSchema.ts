import {
    FilterSchema,
    orderByOf,
    type OrderBySchema,
    type OrderSchema,
    type QuerySchema,
    withQuerySchema
} from "@use-pico/query";
import {
    type IRepository,
    withRepositorySchema
} from "@use-pico/repository";
import {
    type ListSchema,
    merge,
    type ObjectSchema,
    type PicoSchema,
    type RecordSchema,
    type WithIdentitySchema
} from "@use-pico/schema";
import {
    type MutationSchema,
    withMutationSchema
} from "@use-pico/source";

export namespace dullSchema {
    export interface Props<
        TEntity extends WithIdentitySchema,
        TShapeSchema extends ObjectSchema<any>,
        TFilterSchema extends ObjectSchema<any>,
        TOrderBy extends ListSchema.Values,
    > {
        entity: TEntity;
        shape: TShapeSchema;
        filter: TFilterSchema;
        orderBy: TOrderBy;
    }

    export interface Schema<
        TEntity extends WithIdentitySchema,
        TShapeSchema extends ObjectSchema<any>,
        TFilterSchema extends ObjectSchema<any>,
        TOrderBy extends ListSchema.Values,
        TFilterOutput extends FilterSchema = ObjectSchema<
            FilterSchema["shape"] & TFilterSchema["shape"]
        >,
        TOrderByOutput extends OrderBySchema = RecordSchema<ListSchema<TOrderBy, TOrderBy[number]>, OrderSchema>,
        TQueryOutput extends QuerySchema<TFilterOutput, TOrderByOutput> = QuerySchema<
            TFilterOutput,
            TOrderByOutput
        >,
        TMutationOutput extends MutationSchema<TShapeSchema, TQueryOutput> = MutationSchema<
            TShapeSchema,
            TQueryOutput
        >,
    > {
        entity: TEntity;
        shape: TShapeSchema;
        filter: TFilterOutput;
        orderBy: TOrderByOutput;
        query: TQueryOutput;
        mutation: TMutationOutput;
        repository: IRepository.Schema<
            TEntity,
            TShapeSchema,
            TQueryOutput,
            TMutationOutput
        >;
    }

    export namespace Infer {
        export type Entity<
            TSchema extends Schema<any, any, any, any>
        > = PicoSchema.Output<TSchema["entity"]>;

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

        export type Repository<
            TSchema extends Schema<any, any, any, any>
        > = TSchema["repository"];
    }
}

export const dullSchema = <
    TEntity extends WithIdentitySchema,
    TShapeSchema extends ObjectSchema<any>,
    TFilterSchema extends ObjectSchema<any>,
    TOrderBy extends ListSchema.Values,
>(
    {
        entity,
        shape,
        filter,
        orderBy,
    }: dullSchema.Props<
        TEntity,
        TShapeSchema,
        TFilterSchema,
        TOrderBy
    >
): dullSchema.Schema<
    TEntity,
    TShapeSchema,
    TFilterSchema,
    TOrderBy
> => {
    const $filter = merge([
        FilterSchema,
        filter,
    ]);
    const $orderBy = orderByOf(orderBy);
    const $query = withQuerySchema({
        filter:  $filter,
        orderBy: $orderBy,
    });
    const $mutation = withMutationSchema({
        query: $query,
        shape,
    });

    return {
        entity,
        shape,
        filter:     $filter,
        orderBy:    $orderBy,
        query:      $query,
        mutation:   $mutation,
        repository: withRepositorySchema({
            entity,
            shape,
            query:    $query,
            mutation: $mutation,
        }),
    };
};
