import {withUuidTable} from "@use-pico/orm";
import {
    Kysely,
    sql
}                      from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Kingdom", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("created", "timestamp", col =>
            col.notNull().defaultTo(sql`now()`)
        )
        .addUniqueConstraint("Kingdom_name_userId_unique", ["name", "userId"])
        .execute();
}
