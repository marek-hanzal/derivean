import {
    drizzle,
    type PostgresJsDatabase
}                     from "drizzle-orm/postgres-js";
import postgres       from "postgres";
import {type IConfig} from "../api/IConfig";
import {type IOrm}    from "../api/IOrm";
import {withConfig}   from "../container/withConfig";

export class Orm implements IOrm {
    static inject = [
        withConfig.key,
    ];

    protected connection: PostgresJsDatabase<Record<string, never>> | undefined;

    constructor(
        protected config: IConfig,
    ) {
    }

    public orm(): PostgresJsDatabase<Record<string, never>> {
        return this.connection ?? (this.connection = drizzle(
            postgres(this.config.dsn, {
                max: 10,
                ...this.config.options,
            })
        ));
    }
}
