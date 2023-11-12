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
import {BuildingSchema}     from "../schema/BuildingSchema";

export class BuildingRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<BuildingSchema>,
    "Building"
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
            BuildingSchema.repository,
            "Building",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name:       "name",
            producerId: "producerId",
        };
    }

    public async toCreate(create: NonNullable<PicoSchema.Output<withDullSchema.Infer.RepositorySchema<BuildingSchema>["mutation"]["shape"]["create"]>>): Promise<Omit<withDullSchema.Infer.Entity<BuildingSchema>, "id">> {
        return {
            ...create,
            producerId: create.producerId || (await this.producerRepository.withMutation.create({
                name: create.name,
                time: 30,
            })).id,
        };
    }
}

export namespace BuildingRepository {
    export type Type = InstanceType<typeof BuildingRepository>;
}
