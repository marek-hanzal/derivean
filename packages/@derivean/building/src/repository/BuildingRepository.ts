import {type Database}       from "@derivean/orm";
import {
    ProducerRepository,
    withProducerRepository
}                            from "@derivean/producer";
import {lazyOf}              from "@use-pico/container";
import {type withDullSchema} from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {AbstractRepository}  from "@use-pico/repository";
import {BuildingSchema}      from "../schema/BuildingSchema";

export class BuildingRepository extends AbstractRepository<
    Database,
    BuildingSchema,
    "Building"
> {
    static inject = [
        lazyOf(withClient.inject),
        lazyOf(withProducerRepository.inject),
    ];

    constructor(
        client: Client<Database>,
        protected producerRepository: ProducerRepository.Type,
    ) {
        super(
            client,
            BuildingSchema,
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

    public async toCreate(create: withDullSchema.Infer.Create<BuildingSchema>): Promise<withDullSchema.Infer.EntityWithoutId<BuildingSchema>> {
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
