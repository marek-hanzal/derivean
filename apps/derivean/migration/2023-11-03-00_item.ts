import {withUuidTable} from "@use-pico/migrator";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("ItemType", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .execute();

    await withUuidTable("Item", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("typeId", "uuid", col =>
            col.references("ItemType.id").onDelete("cascade").notNull()
        )
        .addUniqueConstraint("Item_name_typeId_unique", ["name", "typeId"])
        .execute();
}
