import {
    type Database,
    withConnection
}                            from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                            from "@use-pico/server";
import {EventInstanceSchema} from "../schema/EventInstanceSchema";

export class EventInstanceRepository extends AbstractRepository<
    Database,
    EventInstanceSchema,
    "EventInstance"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
            EventInstanceSchema,
            "EventInstance",
        );
        this.defaultOrderBy = {
            from: "asc",
        };
        this.matchOf = {
            kingdomId: "kingdomId",
            instant:   "instant",
            commit:    "commit",
            eventId:   "eventId",
            userId:    "userId",
        };
    }
}

export namespace EventInstanceRepository {
    export type Type = InstanceType<typeof EventInstanceRepository>;
}
