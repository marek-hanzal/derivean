import {type Database}           from "@use-pico/orm";
import {
    type FilterSchema,
    type OrderBySchema,
    type OrderSchema,
    type QuerySchema
}                                from "@use-pico/query";
import {type PicoSchema}         from "@use-pico/schema";
import {type SelectQueryBuilder} from "kysely";
import {type IRepository}        from "../api/IRepository";
import {type IWithApply}         from "../api/IWithApply";

export abstract class AbstractWithApply<
    TDatabase extends Database,
    TSchema extends IRepository.Schema<any, any, QuerySchema<FilterSchema, OrderBySchema>, any>,
    TTable extends keyof TDatabase & string,
> implements IWithApply<
    TDatabase,
    TSchema,
    TTable
> {
    public defaultOrderBy?: PicoSchema.Output<TSchema["query"]["shape"]["orderBy"]>;
    public matchOf?: Record<
        keyof Omit<
            NonNullable<PicoSchema.Output<TSchema["query"]["shape"]["where"]>>,
            keyof FilterSchema.Type
        >,
        string
    >;

    protected constructor(
        public schema: TSchema,
        public table: TTable,
    ) {
    }

    public applyWhere<T>(
        query: PicoSchema.Output<TSchema["query"]>,
        select: SelectQueryBuilder<TDatabase, TTable, T>
    ): SelectQueryBuilder<TDatabase, TTable, T> {
        let $select = select;

        const $matchOf = {
            /**
             * Default fields; it assumes standard ID field is present
             */
            id: "id",
            ...this.matchOf,
        };

        /**
         * A bit of magic: look into fields coming from the outside and remap them on exact match for database column.
         *
         * This is because we don't want to let user directly send column names here.
         *
         * Also, this enables search only for known fields, not arbitrary ones.
         */
        for (const [match, value] of Object.entries(query.where || {})) {
            $matchOf[match as keyof typeof this.matchOf] && ($select = $select.where($matchOf[match as keyof typeof this.matchOf] as any, "=", value));
        }

        return $select;
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

        for (const [column, order] of Object.entries(query.orderBy || this.defaultOrderBy || {})) {
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
