import {
    InventoryRepository,
    withInventoryRepository
}                                     from "@derivean/inventory";
import {type Database}                from "@derivean/orm";
import {lazyOf}                       from "@use-pico/container";
import {withDullSchema}               from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                                     from "@use-pico/orm";
import {AbstractRepository}           from "@use-pico/repository";
import {withEventInventoryRepository} from "../container/withEventInventoryRepository";
import {EventSchema}                  from "../schema/EventSchema";
import {EventInventoryRepository}     from "./EventInventoryRepository";

export class EventRepository extends AbstractRepository<
    Database,
    EventSchema,
    "Event"
> {
    static inject = [
        lazyOf(withClient.inject),
        lazyOf(withInventoryRepository.inject),
        lazyOf(withEventInventoryRepository.inject),
    ];

    constructor(
        client: Client<Database>,
        protected readonly inventoryRepository: InventoryRepository.Type,
        protected readonly eventInventoryRepository: EventInventoryRepository.Type,
    ) {
        super(
            client,
            EventSchema,
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

    public async onCreate(entity: withDullSchema.Infer.Entity<EventSchema>): Promise<any> {
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
