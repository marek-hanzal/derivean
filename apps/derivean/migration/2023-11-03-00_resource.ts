import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Resource", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("type", "varchar(64)", col =>
            col.notNull()
        )
        .addUniqueConstraint("name_type", ["name", "type"])
        .execute();
}
