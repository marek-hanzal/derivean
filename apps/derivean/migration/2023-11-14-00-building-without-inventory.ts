import {Kysely} from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await db.schema
        .alterTable("Building")
        .dropColumn("inventoryId")
        .execute();
}
