import {
    type Client,
    type Database
}                         from "@use-pico2/orm";
import {
    type CountSchema,
    type FilterSchema,
    type OrderBySchema,
    type QuerySchema
}                         from "@use-pico2/query";
import {type IRepository} from "@use-pico2/repository";
import {
    type PicoSchema,
    type WithIdentitySchema
}                         from "@use-pico2/schema";
import {
    type MutationSchema,
    type ShapeSchema
}                         from "@use-pico2/source";

export abstract class AbstractRepository<
    TEntitySchema extends WithIdentitySchema,
    TShapeSchema extends ShapeSchema,
    TQuerySchema extends QuerySchema<FilterSchema, OrderBySchema>,
    TMutationSchema extends MutationSchema<TShapeSchema, TQuerySchema>,
    TDatabase extends Database,
> implements IRepository<
    TEntitySchema,
    TShapeSchema,
    TQuerySchema,
    TMutationSchema,
    TDatabase
> {
    protected constructor(
        readonly entitySchema: TEntitySchema,
        readonly shapeSchema: TShapeSchema,
        readonly querySchema: TQuerySchema,
        readonly mutationSchema: TMutationSchema,
        readonly client: Client<any>,
        readonly table: keyof TDatabase & string,
    ) {
    }

    public async count(query: PicoSchema.Output<TQuerySchema>): Promise<CountSchema.Type> {
        return {
            total: parseInt(
                (
                    await this.client.selectFrom(this.table).select(({fn}) => [
                        fn.count<number>("id").as("total")
                    ]).executeTakeFirstOrThrow()
                ).total as unknown as string
            ),

            where: parseInt(
                (
                    await this.client.selectFrom(this.table).select(({fn}) => [
                        fn.count<number>("id").as("total")
                    ]).executeTakeFirstOrThrow()
                ).total as unknown as string
            ),

            count: parseInt(
                (
                    await this.client.selectFrom(this.table).select(({fn}) => [
                        fn.count<number>("id").as("total")
                    ]).executeTakeFirstOrThrow()
                ).total as unknown as string
            ),
        };
    }

    public async query(query: PicoSchema.Output<TQuerySchema>): Promise<PicoSchema.Output<TEntitySchema>[]> {
        return this.client.selectFrom(this.table).selectAll().execute() as unknown as Promise<PicoSchema.Output<TEntitySchema>[]>;
    }

    public mutate(mutation: PicoSchema.Output<TMutationSchema>): Promise<PicoSchema.Output<TEntitySchema>> {
        console.log("mutation", mutation);
        return Promise.resolve(undefined as any);
    }
}
