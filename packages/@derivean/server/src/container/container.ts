import {withProducerContainer} from "@derivean/producer";
import {withResourceContainer} from "@derivean/resource";
import {withClient}            from "@use-pico2/orm";
import {withServerContainer}   from "@use-pico2/server";
import {
    Kysely,
    PostgresDialect
}                              from "kysely";
import {Pool}                  from "pg";

const register = [
    withProducerContainer,
    withResourceContainer,
] as const;

export const container = withServerContainer();
withClient.factory(container, () => {
    return new Kysely({
        dialect: new PostgresDialect({
            pool: new Pool({
                connectionString: process.env.DATABASE_URL,
            }),
        }),
    });
});

register.forEach(register => register(container));
