import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {EventHeroSchema}    from "../schema/EventHeroSchema";

export class EventHeroRepository extends AbstractRepository<
    Database,
    EventHeroSchema,
    "EventHero"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            EventHeroSchema,
            "EventHero",
        );
        this.matchOf = {
            eventId: "eventId",
        };
    }
}

export namespace EventHeroRepository {
    export type Type = InstanceType<typeof EventHeroRepository>;
}
