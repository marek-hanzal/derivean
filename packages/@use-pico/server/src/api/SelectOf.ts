import {type SelectQueryBuilder} from "kysely";
import {type Database}           from "./Database";

export type SelectOf<
    TDatabase extends Database,
    TTable extends keyof TDatabase & string,
    T,
> = SelectQueryBuilder<TDatabase, TTable, T>;
