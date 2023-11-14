import {type Database}        from "@derivean/orm";
import {lazyOf}               from "@use-pico/container";
import {
    type Client,
    withClient
}                             from "@use-pico/orm";
import {AbstractRepository}   from "@use-pico/repository";
import {EventInventorySchema} from "../schema/EventInventorySchema";

export class EventInventoryRepository extends AbstractRepository<
    Database,
    EventInventorySchema,
    "EventInventory"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
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
