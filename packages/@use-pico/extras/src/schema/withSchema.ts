import {
    type FilterSchema,
    type OrderBySchema,
    withMutationSchema,
    withQuerySchema
}                    from "@use-pico/query";
import {
    type ObjectSchema,
    type WithIdentitySchema
}                    from "@use-pico/schema";
import {type Schema} from "../api/Schema";

export namespace withSchema {
    export interface Props<
        TEntity extends WithIdentitySchema,
        TShapeSchema extends ObjectSchema<any>,
        TFilterSchema extends FilterSchema,
        TOrderBySchema extends OrderBySchema,
    > {
        /**
         * Define shape of an entity (usually shape of database table or an output from a Repository)
         */
        entity: TEntity;
        /**
         * Working shape of an entity outside of Database (usually when creating/updating one)
         */
        shape: TShapeSchema;
        /**
         * Define available filters of a Repository (used by a client)
         */
        filter: TFilterSchema;
        /**
         * Define available sorting of a Repository (used by a client)
         */
        orderBy: TOrderBySchema;
    }
}

/**
 * The Whole library is based on top of a few schemas. This is a factory function to create them.
 */
export const withSchema = <
    TEntity extends WithIdentitySchema,
    TShapeSchema extends ObjectSchema<any>,
    TFilterSchema extends FilterSchema,
    TOrderBySchema extends OrderBySchema,
>(
    {
        entity,
        shape,
        filter,
        orderBy,
    }: withSchema.Props<
        TEntity,
        TShapeSchema,
        TFilterSchema,
        TOrderBySchema
    >
): Schema<
    TEntity,
    TShapeSchema,
    TFilterSchema,
    TOrderBySchema
> => {
    const query = withQuerySchema({
        filter,
        orderBy,
    });
    const mutation = withMutationSchema({
        query,
        shape,
    });

    return {
        entity,
        shape,
        filter,
        orderBy,
        query,
        mutation,
    };
};
