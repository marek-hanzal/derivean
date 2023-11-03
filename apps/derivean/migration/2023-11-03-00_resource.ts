import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Resource", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .execute();
}
