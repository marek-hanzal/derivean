import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("ResourceType", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .execute();

    await withUuidTable("Resource", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("typeId", "uuid", col =>
            col.references("ResourceType.id").onDelete("cascade").notNull()
        )
        .addUniqueConstraint("name_type", ["name", "typeId"])
        .execute();
}
