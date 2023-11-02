import {type Database}           from "@use-pico/orm";
import {
    OrderSchema,
    type QuerySchema
}                                from "@use-pico/query";
import {type PicoSchema}         from "@use-pico/schema";
import {type SelectQueryBuilder} from "kysely";
import {type IRepository}        from "../api/IRepository";
import {type IWithApply}         from "../api/IWithApply";

export abstract class AbstractWithApply<
    TDatabase extends Database,
    TSchema extends IRepository.Schema<any, any, QuerySchema<any, any>, any>,
    TTable extends keyof TDatabase & string,
> implements IWithApply<
    TDatabase,
    TSchema,
    TTable
> {
    protected constructor(
        public schema: TSchema,
        public table: TTable,
        public defaultOrderBy: PicoSchema.Output<TSchema["query"]["shape"]["orderBy"]>,
    ) {
    }

    public applyWhere<T>(
        query: PicoSchema.Output<TSchema["query"]>,
        select: SelectQueryBuilder<TDatabase, TTable, T>
    ): SelectQueryBuilder<TDatabase, TTable, T> {
        return select;
    }

    public applyFilter<T>(
        query: PicoSchema.Output<TSchema["query"]>,
        select: SelectQueryBuilder<TDatabase, TTable, T>
    ): SelectQueryBuilder<TDatabase, TTable, T> {
        return select;
    }

    public applyOrderBy<T>(
        query: PicoSchema.Output<TSchema["query"]>,
        select: SelectQueryBuilder<TDatabase, TTable, T>
    ): SelectQueryBuilder<TDatabase, TTable, T> {
        let $select = select;

        for (const [column, order] of Object.entries(query.orderBy || this.defaultOrderBy)) {
            $select = $select.orderBy(column as any, order as OrderSchema.Type);
        }

        return $select;
    }

    public applyTo<T>(
        query: PicoSchema.Output<TSchema["query"]>,
        select: SelectQueryBuilder<TDatabase, TTable, T>
    ): SelectQueryBuilder<TDatabase, TTable, T> {
        let $select = this.applyFilter(
            query,
            this.applyWhere(
                query,
                select
            )
        );

        query.cursor && ($select = $select.limit(query.cursor.size).offset(query.cursor.page * query.cursor.size));

        return this.applyOrderBy(query, $select);
    }
}
