import {defaultMigration} from "@use-pico/server";
import {Kysely}           from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    return defaultMigration(db);
}
