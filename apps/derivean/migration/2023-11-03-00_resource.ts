import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("ResourceType", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .execute();

    const resourceTypeId = await db.insertInto("ResourceType")
        .values({
            name: "resource",
        })
        .returningAll()
        .executeTakeFirstOrThrow();

    await db.insertInto("ResourceType")
        .values([
            {
                name: "building",
            }
        ])
        .execute();

    await withUuidTable("Resource", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull()
        )
        .addColumn("typeId", "uuid", col =>
            col.references("ResourceType.id").onDelete("cascade").notNull()
        )
        .addUniqueConstraint("Resource_name_type_unique", ["name", "typeId"])
        .execute();

    await db.insertInto("Resource")
        .values([
            {
                name:   "gold",
                typeId: resourceTypeId.id,
            },
            {
                name:   "coin",
                typeId: resourceTypeId.id,
            },
            {
                name:   "bread",
                typeId: resourceTypeId.id,
            },
            {
                name:   "corn",
                typeId: resourceTypeId.id,
            },
            {
                name:   "beer",
                typeId: resourceTypeId.id,
            },
            {
                name:   "log",
                typeId: resourceTypeId.id,
            },
            {
                name:   "plank",
                typeId: resourceTypeId.id,
            },
            {
                name:   "stone",
                typeId: resourceTypeId.id,
            },
            {
                name:   "iron",
                typeId: resourceTypeId.id,
            },
            {
                name:   "pig",
                typeId: resourceTypeId.id,
            },
            {
                name:   "sausage",
                typeId: resourceTypeId.id,
            },
            {
                name:   "flour",
                typeId: resourceTypeId.id,
            },
            {
                name:   "coal",
                typeId: resourceTypeId.id,
            },
            {
                name:   "water",
                typeId: resourceTypeId.id,
            },
            {
                name:   "abalone",
                typeId: resourceTypeId.id,
            },
        ])
        .execute();
}
