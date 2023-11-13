import {withUuidTable} from "@use-pico/migrator";
import {
    Kysely,
    sql
}                      from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Kingdom", db)
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("created", "timestamp", col =>
            col.notNull().defaultTo(sql`now()`)
        )
        .addUniqueConstraint("Kingdom_name_userId_unique", ["name", "userId"])
        .execute();
}
