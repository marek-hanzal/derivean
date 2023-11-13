import {withUuidTable} from "@use-pico/orm";
import {Kysely}        from "kysely";

export async function up(db: Kysely<any>): Promise<void> {
    await withUuidTable("Building", db)
        .addColumn("name", "varchar(64)", col =>
            col.notNull().unique()
        )
        .addColumn("construction", "float4", col =>
            col.notNull()
        )
        .addColumn("producerId", "uuid", col =>
            col.references("Producer.id").onDelete("cascade").notNull()
        )
        .addColumn("inventoryId", "uuid", col =>
            col.references("Inventory.id").onDelete("cascade").notNull()
        )
        .execute();

    await db.insertInto("Building")
        .values([
            {
                name:       "lumberjack",
                producerId: (await db.insertInto("Producer").values({
                    name: "lumberjack",
                    time: 10,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "quarry",
                producerId: (await db.insertInto("Producer").values({
                    name: "quarry",
                    time: 15,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "sawmill",
                producerId: (await db.insertInto("Producer").values({
                    name: "sawmill",
                    time: 20,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "bakery",
                producerId: (await db.insertInto("Producer").values({
                    name: "bakery",
                    time: 12,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "butcher",
                producerId: (await db.insertInto("Producer").values({
                    name: "butcher",
                    time: 30,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "windmill",
                producerId: (await db.insertInto("Producer").values({
                    name: "windmill",
                    time: 12,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "coal-mine",
                producerId: (await db.insertInto("Producer").values({
                    name: "coal-mine",
                    time: 15,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "gold-mine",
                producerId: (await db.insertInto("Producer").values({
                    name: "gold-mine",
                    time: 30,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "farm",
                producerId: (await db.insertInto("Producer").values({
                    name: "farm",
                    time: 30,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "pig-farm",
                producerId: (await db.insertInto("Producer").values({
                    name: "pig-farm",
                    time: 45,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "well",
                producerId: (await db.insertInto("Producer").values({
                    name: "well",
                    time: 30,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "goldsmith",
                producerId: (await db.insertInto("Producer").values({
                    name: "goldsmith",
                    time: 45,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
            {
                name:       "tavern",
                producerId: (await db.insertInto("Producer").values({
                    name: "tavern",
                    time: 60,
                }).returning("id").executeTakeFirstOrThrow()).id,
            },
        ])
        .execute();


    const inputs = [
        {
            producer: "lumberjack",
            resource: "bread",
            type:     "resource",
            amount:   1,
        },
        {
            producer: "farm",
            resource: "water",
            type:     "resource",
            amount:   6,
        },
        {
            producer: "quarry",
            resource: "bread",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "sawmill",
            resource: "bread",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "sawmill",
            resource: "log",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "bakery",
            resource: "flour",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "windmill",
            resource: "corn",
            type:     "resource",
            amount:   10,
        },
        {
            producer: "butcher",
            resource: "pig",
            type:     "resource",
            amount:   1,
        },
        {
            producer: "butcher",
            resource: "bread",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "pig-farm",
            resource: "water",
            type:     "resource",
            amount:   8,
        },
        {
            producer: "pig-farm",
            resource: "corn",
            type:     "resource",
            amount:   10,
        },
        {
            producer: "bakery",
            resource: "water",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "tavern",
            resource: "sausage",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "gold-mine",
            resource: "sausage",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "coal-mine",
            resource: "beer",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "gold-mine",
            resource: "beer",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "tavern",
            resource: "bread",
            type:     "resource",
            amount:   6,
        },
        {
            producer: "tavern",
            resource: "beer",
            type:     "resource",
            amount:   6,
        },
    ] as const;

    const outputs = [
        {
            producer: "well",
            resource: "water",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "lumberjack",
            resource: "log",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "sawmill",
            resource: "plank",
            type:     "resource",
            amount:   12,
        },
        {
            producer: "farm",
            resource: "corn",
            type:     "resource",
            amount:   30,
        },
        {
            producer: "quarry",
            resource: "stone",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "windmill",
            resource: "flour",
            type:     "resource",
            amount:   6,
        },
        {
            producer: "bakery",
            resource: "bread",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "butcher",
            resource: "abalone",
            type:     "resource",
            amount:   1,
        },
        {
            producer: "butcher",
            resource: "sausage",
            type:     "resource",
            amount:   12,
        },
        {
            producer: "coal-mine",
            resource: "coal",
            type:     "resource",
            amount:   10,
        },
        {
            producer: "gold-mine",
            resource: "gold",
            type:     "resource",
            amount:   8,
        },
        {
            producer: "pig-farm",
            resource: "pig",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "tavern",
            resource: "coin",
            type:     "resource",
            amount:   20,
        },
    ] as const;

    for (const {
        producer,
        resource,
        type,
        amount,
    } of inputs) {
        await db.insertInto("ProducerInput")
            .values([
                {
                    producerId: (await db.selectFrom("Producer").select("id").where("name", "=", producer).executeTakeFirstOrThrow()).id,
                    resourceId: (await db.selectFrom("Resource").innerJoin("ResourceType", "ResourceType.id", "Resource.typeId").select("Resource.id").where("Resource.name", "=", resource).where("ResourceType.name", "=", type).executeTakeFirstOrThrow()).id,
                    amount,
                },
            ])
            .execute();
    }

    for (const {
        producer,
        resource,
        type,
        amount,
    } of outputs) {
        await db.insertInto("ProducerOutput")
            .values([
                {
                    producerId: (await db.selectFrom("Producer").select("id").where("name", "=", producer).executeTakeFirstOrThrow()).id,
                    resourceId: (await db.selectFrom("Resource").innerJoin("ResourceType", "ResourceType.id", "Resource.typeId").select("Resource.id").where("Resource.name", "=", resource).where("ResourceType.name", "=", type).executeTakeFirstOrThrow()).id,
                    amount,
                },
            ])
            .execute();
    }

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
        .addColumn("created", "datetime", col =>
            col.notNull()
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
