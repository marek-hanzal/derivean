import {withUuidTable} from "@use-pico/migrator/src/migrator/withUuidTable";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("EventHero", db)
        .addColumn("eventId", "uuid", col =>
            col.references("Event.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "integer", col => col.notNull())
        .execute();
}
