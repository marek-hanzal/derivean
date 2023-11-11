import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Building", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .execute();

    await db.insertInto("Building")
        .values([
            {
                name: "lumberjack",
            },
            {
                name: "quarry",
            },
            {
                name: "sawmill",
            },
            {
                name: "bakery",
            },
            {
                name: "butcher",
            },
            {
                name: "windmill",
            },
            {
                name: "coal mine",
            },
            {
                name: "gold mine",
            },
            {
                name: "farm",
            },
            {
                name: "pig farm",
            },
            {
                name: "well",
            },
            {
                name: "tavern",
            },
        ])
        .execute();

    /**
     * Building can have more producers, for example:
     *
     * Lumberjack can produce logs AND pay taxes, thus, one producer for logs, another for taxes.
     */
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
        .addColumn("resourceId", "uuid", col =>
            col.references("Resource.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("BuildingInstance", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("level", "integer", col => col.notNull())
        .execute();

}
