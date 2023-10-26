import {type PostgresJsDatabase} from "drizzle-orm/postgres-js";

export interface IOrm {
    orm(): PostgresJsDatabase<Record<string, never>>;
}
