import {
    type Infer,
    type Schema
}                          from "@use-pico/extras";
import {type FilterSchema} from "@use-pico/query";
import {type Database}     from "./Database";
import {type SelectOf}     from "./SelectOf";

export interface IWithApply<
    TDatabase extends Database,
    TSchema extends Schema<any, any, any, any>,
    TTable extends keyof TDatabase & string,
> {
    readonly schema: TSchema;
    readonly table: TTable;
    defaultOrderBy?: Infer.OrderBy<TSchema>;
    matchOf?: Record<
        keyof Omit<
            Infer.Filter<TSchema>,
            keyof FilterSchema.Type
        >,
        string
    >;
    matchOfIn?: Partial<
        Record<
            keyof Omit<
                Infer.Filter<TSchema>,
                keyof FilterSchema.Type
            >,
            string
        >
    >;
    fulltextOf?: Partial<
        Record<
            keyof Omit<
                Infer.Filter<TSchema>,
                keyof FilterSchema.Type
            >,
            string
        >
    >;

    /**
     * Prepare a query builder "before" any other stuff is applied.
     */
    with<T>(
        query: Infer.Query<TSchema>,
        select: SelectOf<TDatabase, TTable, T>
    ): SelectOf<TDatabase, TTable, T>;

    /**
     * Apply mandatory filter which cannot be modified (like userId or so)
     */
    applyWhere<T>(
        query: Infer.Query<TSchema>,
        select: SelectOf<TDatabase, TTable, T>
    ): SelectOf<TDatabase, TTable, T>;

    /**
     * Apply optional user-land filters
     */
    applyFilter<T>(
        query: Infer.Query<TSchema>,
        select: SelectOf<TDatabase, TTable, T>
    ): SelectOf<TDatabase, TTable, T>;

    /**
     * Apply orderBy from a query or a default one
     */
    applyOrderBy<T>(
        query: Infer.Query<TSchema>,
        select: SelectOf<TDatabase, TTable, T>
    ): SelectOf<TDatabase, TTable, T>;

    /**
     * Apply all available "stuff" on the given query: where, filter, order by, limits, ...
     */
    applyTo<T>(
        query: Infer.Query<TSchema>,
        select: SelectOf<TDatabase, TTable, T>
    ): SelectOf<TDatabase, TTable, T>;
}
