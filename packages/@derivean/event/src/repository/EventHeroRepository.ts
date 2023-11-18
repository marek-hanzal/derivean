import {
    type Database,
    withConnection
}                        from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                        from "@use-pico/server";
import {EventHeroSchema} from "../schema/EventHeroSchema";

export class EventHeroRepository extends AbstractRepository<
    Database,
    EventHeroSchema,
    "EventHero"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
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
