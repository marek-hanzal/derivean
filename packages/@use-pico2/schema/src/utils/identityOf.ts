import {
    merge,
    type ObjectSchema,
    schema,
    WithIdentitySchema
} from "@use-pico2/schema";

export namespace identityOf {
    export type Props<
        TSchema extends ObjectSchema<any>,
    > = (factory: schema.Schema) => TSchema;
}

export const identityOf = <
    TSchema extends ObjectSchema<any>,
>(
    factory: identityOf.Props<TSchema>,
) => {
    return merge([
        WithIdentitySchema,
        schema(factory),
    ]);
};
