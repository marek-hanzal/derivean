import {
    type Infer,
    type Schema
}                         from "@use-pico/extras";
import {type CountSchema} from "@use-pico/query";
import {
    type SelectExpression,
    type Selection
}                         from "kysely";
import {type Connection}  from "./Connection";
import {type Database}    from "./Database";
import {type IRepository} from "./IRepository";
import {type SelectOf}    from "./SelectOf";

export interface IWithQuery<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> {
    readonly connection: Connection<TDatabase>;
    readonly schema: TSchema;
    readonly table: TTable;
    readonly repository: IRepository<TDatabase, TSchema, TTable>;

    count(query: Infer.Query<TSchema>): Promise<CountSchema.Type>;

    /**
     * Query collection of items
     */
    query(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema>[]>;

    /**
     * Fetch an (optional) item
     */
    fetch(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema> | undefined>;

    /**
     * Fetch an item, throw if not found
     */
    fetchOrThrow(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema>>;

    select<
        TExpression extends SelectExpression<TDatabase, TTable>
    >(
        selections?: ReadonlyArray<TExpression>
    ): SelectOf<
        TDatabase,
        TTable,
        Selection<TDatabase, TTable, TExpression>
    >;
}
