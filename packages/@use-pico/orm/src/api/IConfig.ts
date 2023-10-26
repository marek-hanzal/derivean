import postgres from "postgres";

export interface IConfig {
    dsn: string;
    options?: postgres.Options<any>;
}
