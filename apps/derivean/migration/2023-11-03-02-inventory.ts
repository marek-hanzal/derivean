import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Inventory", db)
        .execute();

    await withUuidTable("InventoryItem", db)
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .addColumn("itemId", "uuid", col =>
            col.references("Item.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .addColumn("limit", "float4", col => col.notNull())
        .execute();
}
