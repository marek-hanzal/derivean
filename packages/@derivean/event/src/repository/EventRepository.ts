import {withInventoryRepository}      from "@derivean/inventory";
import {
    type Database,
    withConnection
}                                     from "@derivean/orm";
import {type Infer}                   from "@use-pico/extras";
import {
    AbstractRepository,
    lazyOf
}                                     from "@use-pico/server";
import {withEventHeroRepository}      from "../container/withEventHeroRepository";
import {withEventInventoryRepository} from "../container/withEventInventoryRepository";
import {EventSchema}                  from "../schema/EventSchema";

export class EventRepository extends AbstractRepository<
    Database,
    EventSchema,
    "Event"
> {
    static inject = [
        lazyOf(withConnection.inject),
        lazyOf(withInventoryRepository.inject),
        lazyOf(withEventInventoryRepository.inject),
        lazyOf(withEventHeroRepository.inject),
    ];

    constructor(
        connection: withConnection,
        protected inventoryRepository: withInventoryRepository,
        protected eventInventoryRepository: withEventInventoryRepository,
        protected eventHeroRepository: withEventHeroRepository,
    ) {
        super(
            connection,
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

    public async onCreate(entity: Infer.Entity<EventSchema>): Promise<any> {
        switch (entity.type) {
            case "EventInventory":
                await this.eventInventoryRepository.withMutation.create({
                    eventId:     entity.id,
                    inventoryId: (await this.inventoryRepository.withMutation.create({
                        name: `EventInventory ${entity.name}`,
                    })).id,
                });
                break;
            case "EventHero":
                await this.eventHeroRepository.withMutation.create({
                    eventId: entity.id,
                    amount:  1,
                });
                break;
        }
    }
}

export namespace EventRepository {
    export type Type = InstanceType<typeof EventRepository>;
}
