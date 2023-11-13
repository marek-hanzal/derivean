import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Event", db)
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade")
        )
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("type", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("from", "timestamp")
        .addColumn("to", "timestamp")
        .addColumn("duration", "float4")
        .addColumn("instant", "boolean")
        .addUniqueConstraint("Event_name_type_unique", ["name", "type"])
        .execute();

    await withUuidTable("EventItem", db)
        .addColumn("eventId", "uuid", col =>
            col.references("Event.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("EventInstance", db)
        .addColumn("eventId", "uuid", col =>
            col.references("Event.id").onDelete("cascade").notNull()
        )
        .addColumn("kingdomId", "uuid", col =>
            col.references("Kingdom.id").onDelete("cascade").notNull()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("from", "timestamp")
        .addColumn("to", "timestamp")
        .addColumn("commit", "boolean")
        .execute();
}
