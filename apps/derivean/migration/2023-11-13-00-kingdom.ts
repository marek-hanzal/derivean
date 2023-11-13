import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Kingdom", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .execute();

    await db.schema.alterTable("BuildingInstance")
        .addColumn("kingdomId", "uuid", col =>
            col.references("Kingdom.id").onDelete("cascade").notNull()
        )
        .execute();
}
