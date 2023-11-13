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
import {type PicoSchema}    from "@use-pico/schema";
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
            name: "asc",
        };
        this.matchOf = {
            name:       "name",
            producerId: "producerId",
        };
    }

    public async toCreate(create: NonNullable<PicoSchema.Output<withDullSchema.Infer.RepositorySchema<EventSchema>["mutation"]["shape"]["create"]>>): Promise<Omit<withDullSchema.Infer.Entity<EventSchema>, "id">> {
        return {
            ...create,
            producerId: create.producerId || (await this.producerRepository.withMutation.create({
                name: create.name,
                time: 30,
            })).id,
        };
    }
}

export namespace EventRepository {
    export type Type = InstanceType<typeof EventRepository>;
}
