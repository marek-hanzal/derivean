import {type QuerySchema} from "@use-pico2/query";
import {
    type NullishSchema,
    type ObjectSchema,
    type PartialSchema,
    type PicoSchema
}                         from "@use-pico2/schema";
import {type ShapeSchema} from "./ShapeSchema";

export type MutationSchema<
    TShapeSchema extends ShapeSchema,
    TQuerySchema extends QuerySchema<any, any>,
> = ObjectSchema<{
    create: NullishSchema<TShapeSchema>;
    update: NullishSchema<ObjectSchema<{
        update: NullishSchema<
            PartialSchema<TShapeSchema>
        >;
        query: TQuerySchema;
    }>>;
    upsert: NullishSchema<ObjectSchema<{
        create: NullishSchema<TShapeSchema>;
        update: NullishSchema<ObjectSchema<{
            update: NullishSchema<
                PartialSchema<TShapeSchema>
            >;
            query: TQuerySchema;
        }>>;
    }>>;
    delete: NullishSchema<TQuerySchema>;
}>;

export namespace MutationSchema {
    export type Type<
        TShapeSchema extends ShapeSchema,
        TQuerySchema extends QuerySchema<any, any>,
    > = PicoSchema.Output<MutationSchema<TShapeSchema, TQuerySchema>>;
}
