import {
    type Infer,
    type Schema
}                         from "@use-pico/extras";
import {type Connection}  from "./Connection";
import {type Database}    from "./Database";
import {type IRepository} from "./IRepository";

export interface IWithMutation<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> {
    readonly connection: Connection<TDatabase>;
    readonly schema: TSchema;
    readonly table: TTable;
    readonly repository: IRepository<TDatabase, TSchema, TTable>;

    mutation(mutate: Infer.Mutation<TSchema>): Promise<Infer.Entity<TSchema>>;

    create(
        create: Infer.Create<TSchema>
    ): Promise<Infer.Entity<TSchema>>;

    update(
        update: Infer.Update<TSchema>
    ): Promise<Infer.Entity<TSchema>>;

    delete(
        query: Infer.Delete<TSchema>
    ): Promise<Infer.Entity<TSchema>>;
}
