import {
    type Infer,
    type Schema
}                         from "@use-pico/extras";
import {type CountSchema} from "@use-pico/query";
import {
    type SelectExpression,
    type Selection
}                         from "kysely";
import {type Connection}  from "../api/Connection";
import {type Database}    from "../api/Database";
import {type IRepository} from "../api/IRepository";
import {type IWithQuery}  from "../api/IWithQuery";
import {type SelectOf}    from "../api/SelectOf";

export class WithQuery<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> implements IWithQuery<TDatabase, TSchema, TTable> {
    constructor(
        public connection: Connection<TDatabase>,
        public schema: TSchema,
        public table: TTable,
        public repository: IRepository<TDatabase, TSchema, any>,
    ) {
    }

    public async count(query: Infer.Query<TSchema>): Promise<CountSchema.Type> {
        return {
            count: parseInt(
                (
                    await this.repository.applyFilter(
                        query,
                        this.repository.applyWhere(
                            query,
                            this.connection
                                .selectFrom(this.table)
                                .select(({fn}) => [
                                    fn.count("id").as("count")
                                ])
                        )
                    ).executeTakeFirst() as any
                ).count as string
            ),

            where: parseInt(
                (
                    await this.repository.applyWhere(
                        query,
                        this.connection
                            .selectFrom(this.table)
                            .select(({fn}) => [
                                fn.count("id").as("count")
                            ])
                    ).executeTakeFirst() as any
                ).count as string
            ),

            total: parseInt(
                (await this.connection
                    .selectFrom(this.table)
                    .select(({fn}) => [
                        fn.count("id").as("count")
                    ])
                    .executeTakeFirst() as any).count as string
            ),
        };
    }

    public async query(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema>[]> {
        return this.repository.applyTo(
            query,
            this.connection
                .selectFrom(this.table)
                .selectAll()
        ).execute();
    }

    public async fetch(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema> | undefined> {
        return this.repository.applyTo(
            query,
            this.connection.selectFrom(this.table).selectAll()
        ).executeTakeFirst();
    }

    public async fetchOrThrow(query: Infer.Query<TSchema>): Promise<Infer.Entity<TSchema>> {
        return this.repository.applyTo(
            query,
            this.connection.selectFrom(this.table).selectAll()
        ).executeTakeFirstOrThrow();
    }

    public select<
        TExpression extends SelectExpression<TDatabase, TTable>
    >(
        selections?: ReadonlyArray<TExpression>
    ): SelectOf<
        TDatabase,
        TTable,
        Selection<TDatabase, TTable, TExpression>
    > {
        const query = this.connection.selectFrom(this.table);
        return (selections ? query.select(selections as any) : query.selectAll()) as any;
    }
}
