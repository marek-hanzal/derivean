import {
    FilterSchema,
    type OrderBySchema,
    withQuerySchema
}                           from "@use-pico/query";
import {
    merge,
    type ObjectSchema,
    schema as coolSchema
}                           from "@use-pico/schema";
import {type ShapeSchema}   from "../schema/ShapeSchema";
import {withMutationSchema} from "./withMutationSchema";

export namespace withSourceSchema {
    export interface Props<
        TEntity extends ObjectSchema<any>,
        TShapeSchema extends ShapeSchema,
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
    > {
        (schema: typeof coolSchema): Schema<TEntity, TShapeSchema, TFilterSchema, TOrderBySchema>;
    }

    export interface Schema<
        TEntity extends ObjectSchema<any>,
        TShapeSchema extends ShapeSchema,
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
    > {
        entity: TEntity;
        shape: TShapeSchema;
        filter: TFilterSchema;
        orderBy: TOrderBySchema;
    }
}

export const withSourceSchema = <
    TEntity extends ObjectSchema<any>,
    TShapeSchema extends ShapeSchema,
    TFilterSchema extends ObjectSchema<any>,
    TOrderBySchema extends OrderBySchema,
>(factory: withSourceSchema.Props<TEntity, TShapeSchema, TFilterSchema, TOrderBySchema>) => {
    const {shape, entity, orderBy, ...rest} = factory(coolSchema);
    const filter = merge([
        FilterSchema,
        rest.filter,
    ]);
    const query = withQuerySchema({
        filter,
        orderBy,
    });
    return coolSchema(z => z.object({
        mutation: withMutationSchema({
            shape,
            query,
        }),
        entity,
        filter,
        orderBy,
        query,
    }));
};
