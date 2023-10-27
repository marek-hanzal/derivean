import {env}                 from "@/derivean/env";
import {PrismaClient}        from "@prisma/client";
import {withClient}          from "@use-pico/orm";
import {withServerContainer} from "@use-pico/server";

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
