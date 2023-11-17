import {
    type Infer,
    type Schema
}                           from "@use-pico/extras";
import {type Connection}    from "../api/Connection";
import {type Database}      from "../api/Database";
import {type IRepository}   from "../api/IRepository";
import {type IWithMutation} from "../api/IWithMutation";
import {type IWithQuery}    from "../api/IWithQuery";
import {AbstractWithApply}  from "./AbstractWithApply";
import {WithMutation}       from "./WithMutation";
import {WithQuery}          from "./WithQuery";

export class AbstractRepository<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> extends AbstractWithApply<
    TDatabase,
    TSchema,
    TTable
> implements IRepository<
    TDatabase,
    TSchema,
    TTable
> {
    protected constructor(
        protected connection: Connection<TDatabase>,
        schema: TSchema,
        table: TTable,
    ) {
        super(
            schema,
            table,
        );
    }

    protected $withQuery: IWithQuery<TDatabase, TSchema, TTable> | undefined;
    protected $withMutation: IWithMutation<TDatabase, TSchema, TTable> | undefined;

    public get withQuery(): IWithQuery<TDatabase, TSchema, TTable> {
        return this.$withQuery ?? (this.$withQuery = new WithQuery(
            this.connection,
            this.schema,
            this.table,
            this,
        ));
    }

    public get withMutation(): IWithMutation<TDatabase, TSchema, TTable> {
        return this.$withMutation ?? (this.$withMutation = new WithMutation(
            this.connection,
            this.schema,
            this.table,
            this,
        ));
    }

    public async get(id: string): Promise<Infer.Entity<TSchema> | undefined> {
        return this.withQuery.fetch({
            where: {id},
        });
    }

    public async getOrThrow(id: string): Promise<Infer.Entity<TSchema>> {
        return this.withQuery.fetchOrThrow({
            where: {id},
        });
    }

    public async toCreate(create: Infer.Create<TSchema>): Promise<Infer.EntityWithoutId<TSchema>> {
        return create;
    }

    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    public async onCreate(entity: Infer.Entity<TSchema>): Promise<any> {
    }

    public async toUpdate(update: Infer.Update<TSchema>["update"]): Promise<Infer.Entity$<TSchema>> {
        return update;
    }

    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    public async onUpdate(entity: Infer.Entity<TSchema>): Promise<any> {
    }
}
