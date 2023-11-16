import {withUuidTable} from "@use-pico/migrator";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Hero", db)
        .addColumn("userId", "uuid", col =>
            col.references("User.id").onDelete("cascade").notNull()
        )
        .addColumn("kingdomId", "uuid", col =>
            col.references("Kingdom.id").onDelete("cascade").notNull()
        )
        .addColumn("inventoryId", "uuid", col =>
            col.references("Kingdom.id").onDelete("cascade").notNull()
        )
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("health", "integer", col => col.notNull())
        .addColumn("level", "integer", col => col.notNull())
        .addColumn("prestige", "integer", col => col.notNull())
        .execute();

    await withUuidTable("Perk", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("value", "float4", col => col.notNull())
        .addColumn("level", "integer", col => col.notNull())
        .addColumn("order", "integer", col => col.notNull())
        .execute();

    await withUuidTable("HeroPerk", db)
        .addColumn("heroId", "uuid", col =>
            col.references("Hero.id").onDelete("cascade").notNull()
        )
        .addColumn("perkId", "uuid", col =>
            col.references("Perk.id").onDelete("cascade").notNull()
        )
        .addUniqueConstraint("HeroPerk_heroId_perkId_unique", ["heroId", "perkId"])
        .execute();

    await withUuidTable("BuildingPerk", db)
        .addColumn("buildingId", "uuid", col =>
            col.references("Building.id").onDelete("cascade").notNull()
        )
        .addColumn("perkId", "uuid", col =>
            col.references("Perk.id").onDelete("cascade").notNull()
        )
        .addUniqueConstraint("BuildingPerk_buildingId_perkId_unique", ["buildingId", "perkId"])
        .execute();
}
