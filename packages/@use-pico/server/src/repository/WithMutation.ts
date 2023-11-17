import {
    type Infer,
    type Schema
}                           from "@use-pico/extras";
import {type Connection}    from "../api/Connection";
import {type Database}      from "../api/Database";
import {type IRepository}   from "../api/IRepository";
import {type IWithMutation} from "../api/IWithMutation";

export class WithMutation<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> implements IWithMutation<TDatabase, TSchema, TTable> {
    constructor(
        public connection: Connection<TDatabase>,
        public schema: TSchema,
        public table: TTable,
        public repository: IRepository<TDatabase, TSchema, any>,
    ) {
    }

    public async mutation(mutate: Infer.Mutation<TSchema>): Promise<Infer.Entity<TSchema>> {
        if (mutate.create) {
            return this.create(mutate.create);
        } else if (mutate.update) {
            return this.update(mutate.update);
        } else if (mutate.delete) {
            return this.delete(mutate.delete);
        }
        throw new Error("Nothing to mutate.");
    }

    public async create(create: Infer.Create<TSchema>): Promise<Infer.Entity<TSchema>> {
        const entity = await this.connection
            .insertInto(this.table)
            .values(await this.repository.toCreate(create) as Infer.Entity<TSchema>)
            .returningAll()
            .executeTakeFirstOrThrow();
        await this.repository.onCreate(entity);
        return entity;
    }

    public async update(
        {
            update,
            query,
        }: Infer.Update<TSchema>
    ): Promise<Infer.Entity<TSchema>> {
        let entity = await this.repository.withQuery.fetchOrThrow(query);
        if (!update) {
            return entity;
        }
        entity = await this.connection
            .updateTable(this.table)
            .set(await this.repository.toUpdate(update))
            /**
             * Resolve an entity with a query to get an ID being updated
             */
            .where("id", "=", entity.id)
            .returningAll()
            .executeTakeFirstOrThrow() as Infer.Entity<TSchema>;
        await this.repository.onUpdate(entity);
        return entity;
    }

    public async delete(query: Infer.Delete<TSchema>): Promise<Infer.Entity<TSchema>> {
        const entity = await this.repository.withQuery.fetchOrThrow(query);
        await this.connection.deleteFrom(this.table).where("id", "=", entity.id).execute();
        return entity;
    }
}
