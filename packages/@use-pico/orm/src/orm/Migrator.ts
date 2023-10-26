import {migrate}        from "drizzle-orm/postgres-js/migrator";
import {type IMigrator} from "../api/IMigrator";
import {type IOrm}      from "../api/IOrm";
import {withOrm}        from "../container/withOrm";

export class Migrator implements IMigrator {
    static inject = [
        withOrm.key,
    ];

    constructor(
        protected orm: IOrm,
    ) {
    }

    public migrate(): Promise<void> {
        return migrate(this.orm.orm(), {migrationsFolder: "migrations"});
    }
}
