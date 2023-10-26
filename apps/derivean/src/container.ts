import {env}                 from "@/derivean/env";
import {withConfig}          from "@use-pico/orm";
import {withServerContainer} from "@use-pico/server";

export const container = withServerContainer();
withConfig.use(container, {
    dsn: env.DATABASE_URL,
});
