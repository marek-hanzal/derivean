import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Producer", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .addColumn("time", "integer", col => col.notNull())
        .execute();

    await withUuidTable("ProducerInput", db)
        .addColumn("producerId", "uuid", col =>
            col.references("Producer.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("ProducerOutput", db)
        .addColumn("producerId", "uuid", col =>
            col.references("Producer.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();
}
