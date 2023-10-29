import {env}                   from "@/derivean/env";
import {withProducerContainer} from "@derivean/producer";
import {withResourceContainer} from "@derivean/resource";
import {PrismaClient}          from "@prisma/client";
import {withClient}            from "@use-pico2/orm";
import {withServerContainer}   from "@use-pico2/server";

const register = [
    withProducerContainer,
    withResourceContainer,
] as const;

export const container = withServerContainer();
withClient.factory(container, () => {
    return new PrismaClient({
        errorFormat: "pretty",
        log:         env.NODE_ENV === "development"
                         ? [
                // "query",
                "error",
                "warn"
            ] : ["error"],
    });
});

register.forEach(register => register(container));

