import {type Database}        from "@derivean/orm";
import {withDullSchema}       from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                             from "@use-pico/orm";
import {AbstractRepository}   from "@use-pico/repository";
import {EventInventorySchema} from "../schema/EventInventorySchema";

export class EventInventoryRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<EventInventorySchema>,
    "EventInventory"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            EventInventorySchema.repository,
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
