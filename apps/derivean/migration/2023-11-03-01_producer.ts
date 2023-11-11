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
        .addColumn("resourceId", "uuid", col =>
            col.references("Resource.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await withUuidTable("ProducerOutput", db)
        .addColumn("producerId", "uuid", col =>
            col.references("Producer.id").onDelete("cascade").notNull()
        )
        .addColumn("resourceId", "uuid", col =>
            col.references("Resource.id").onDelete("cascade").notNull()
        )
        .addColumn("amount", "float4", col => col.notNull())
        .execute();

    await db.insertInto("Producer")
        .values([
            {
                name: "well",
                time: 30,
            },
            {
                name: "bakery",
                time: 12,
            },
            {
                name: "farm",
                time: 30,
            },
            {
                name: "lumberjack",
                time: 10,
            },
            {
                name: "sawmill",
                time: 20,
            },
            {
                name: "butcher",
                time: 30,
            },
            {
                name: "coal mine",
                time: 15,
            },
            {
                name: "gold mine",
                time: 30,
            },
            {
                name: "pig farm",
                time: 45,
            },
            {
                name: "quarry",
                time: 15,
            },
            {
                name: "windmill",
                time: 12,
            },
            {
                name: "tavern",
                time: 60,
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
            producer: "pig farm",
            resource: "water",
            type:     "resource",
            amount:   8,
        },
        {
            producer: "pig farm",
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
            producer: "gold mine",
            resource: "sausage",
            type:     "resource",
            amount:   3,
        },
        {
            producer: "coal mine",
            resource: "beer",
            type:     "resource",
            amount:   4,
        },
        {
            producer: "gold mine",
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
            producer: "coal mine",
            resource: "coal",
            type:     "resource",
            amount:   10,
        },
        {
            producer: "gold mine",
            resource: "gold",
            type:     "resource",
            amount:   8,
        },
        {
            producer: "pig farm",
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
}
