import {
    type Infer,
    type Schema
}                           from "@use-pico/extras";
import {type Database}      from "./Database";
import {type IWithApply}    from "./IWithApply";
import {type IWithMutation} from "./IWithMutation";
import {type IWithQuery}    from "./IWithQuery";

export interface IRepository<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> extends IWithApply<
    TDatabase,
    TSchema,
    TTable
> {
    get withQuery(): IWithQuery<TDatabase, TSchema, TTable>;

    get withMutation(): IWithMutation<TDatabase, TSchema, TTable>;

    get(id: string): Promise<Infer.Entity<TSchema> | undefined>;

    getOrThrow(id: string): Promise<Infer.Entity<TSchema>>;

    toCreate(create: Infer.Create<TSchema>): Promise<Infer.EntityWithoutId<TSchema>>;

    onCreate(entity: Infer.Entity<TSchema>): Promise<any>;

    toUpdate(update: Infer.Update<TSchema>["update"]): Promise<Partial<Infer.Entity<TSchema>>>;

    onUpdate(entity: Infer.Entity<TSchema>): Promise<any>;
}
