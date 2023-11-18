import {migration} from "@use-pico/server";
import {Kysely}    from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    return migration(db);
}
