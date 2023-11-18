import {
    type Database,
    withConnection
}                             from "@derivean/orm";
import {
    AbstractRepository,
    Connection,
    lazyOf
}                             from "@use-pico/server";
import {EventInventorySchema} from "../schema/EventInventorySchema";

export class EventInventoryRepository extends AbstractRepository<
    Database,
    EventInventorySchema,
    "EventInventory"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: Connection<Database>,
    ) {
        super(
            connection,
            EventInventorySchema,
            "EventInventory",
        );
        this.matchOf = {
            eventId:     "eventId",
            inventoryId: "inventoryId",
        };
    }
}

export namespace EventInventoryRepository {
    export type Type = InstanceType<typeof EventInventoryRepository>;
}
