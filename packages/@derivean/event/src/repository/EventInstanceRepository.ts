import {type Database}       from "@derivean/orm";
import {lazyOf}              from "@use-pico/container";
import {withDullSchema}      from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {AbstractRepository}  from "@use-pico/repository";
import {EventInstanceSchema} from "../schema/EventInstanceSchema";

export class EventInstanceRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<EventInstanceSchema>,
    "EventInstance"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            EventInstanceSchema.repository,
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
