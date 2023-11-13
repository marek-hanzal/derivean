import {type Database}      from "@derivean/orm";
import {
    ProducerRepository,
    withProducerRepository
}                           from "@derivean/producer";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {EventSchema}        from "../schema/EventSchema";

export class EventRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<EventSchema>,
    "Event"
> {
    static inject = [
        withClient.inject,
        withProducerRepository.inject,
    ];

    constructor(
        client: Client<Database>,
        protected producerRepository: ProducerRepository.Type,
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
}

export namespace EventRepository {
    export type Type = InstanceType<typeof EventRepository>;
}
