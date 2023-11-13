import {
    Kysely,
    PostgresDialect
}             from "kysely";
import {Pool} from "pg";

export namespace withKysely {
    export interface Props {
        dsn?: string;
    }
}

export const withKysely = <TDatabase>(
    {
        dsn = process.env.DATABASE_URL,
    }: withKysely.Props
) => {
    return new Kysely<TDatabase>({
        dialect: new PostgresDialect({
            pool: new Pool({
                connectionString: dsn,
                types:            {
                    getTypeParser: () => {
                        return value => value;
                    },
                },
            }),
        }),
    });
};
