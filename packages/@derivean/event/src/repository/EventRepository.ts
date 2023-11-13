import {
    InventoryRepository,
    withInventoryRepository
}                                     from "@derivean/inventory";
import {type Database}                from "@derivean/orm";
import {withDullSchema}               from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                                     from "@use-pico/orm";
import {AbstractRepository}           from "@use-pico/repository";
import {type PicoSchema}              from "@use-pico/schema";
import {withEventInventoryRepository} from "../container/withEventInventoryRepository";
import {EventSchema}                  from "../schema/EventSchema";
import {EventInventoryRepository}     from "./EventInventoryRepository";

export class EventRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<EventSchema>,
    "Event"
> {
    static inject = [
        withClient.inject,
        withInventoryRepository.inject,
        withEventInventoryRepository.inject,
    ];

    constructor(
        client: Client<Database>,
        protected readonly inventoryRepository: InventoryRepository.Type,
        protected readonly eventInventoryRepository: EventInventoryRepository.Type,
    ) {
        super(
            client,
            EventSchema.repository,
            "Event",
        );
        this.defaultOrderBy = {
            from: "asc",
            name: "asc",
        };
        this.matchOf = {
            name:    "name",
            type:    "type",
            instant: "instant",
            userId:  "userId",
        };
    }

    public async onCreate(entity: PicoSchema.Output<withDullSchema.Infer.RepositorySchema<EventSchema>["entity"]>): Promise<any> {
        switch (entity.type) {
            case "EventInventory":
                await this.eventInventoryRepository.withMutation.create({
                    eventId:     entity.id,
                    inventoryId: (await this.inventoryRepository.withMutation.create({
                        name: `EventInventory ${entity.name}`,
                    })).id,
                });
                break;
        }
    }
}

export namespace EventRepository {
    export type Type = InstanceType<typeof EventRepository>;
}
