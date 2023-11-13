import {withUuidTable} from "@use-pico/orm";
import {
    Kysely,
    sql
}                      from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Building", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .addColumn("construction", "float4", col =>
            col.notNull()
        )
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .addColumn("maximum", "integer")
        .execute();

    await withUuidTable("BuildingProducer", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("producerId", "uuid", col =>
            col.references("Producer.id").onDelete("cascade").notNull()
        )
        .execute();

    await withUuidTable("BuildingRequirement", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("BuildingConstructionRequirement", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("BuildingInstance", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("kingdomId", "uuid", col =>
            col.references("Kingdom.id").onDelete("cascade").notNull()
        )
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("created", "timestamp", col =>
            col.notNull().defaultTo(sql`now()`)
        )
        .addColumn("level", "integer", col => col.notNull())
        .execute();

}
