import {type Database}             from "@derivean/orm";
import {
    ProducerRepository,
    withProducerRepository
}                                  from "@derivean/producer";
import {withDullSchema}            from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                                  from "@use-pico/orm";
import {AbstractRepository}        from "@use-pico/repository";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export class BuildingRequirementRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<BuildingRequirementSchema>,
    "BuildingRequirement"
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
            BuildingRequirementSchema.repository,
            "BuildingRequirement",
        );
        this.defaultOrderBy = {
            buildingId: "asc",
        };
        this.matchOf = {
            itemId: "itemId",
            buildingId: "buildingId",
        };
    }
}

export namespace BuildingRequirementRepository {
    export type Type = InstanceType<typeof BuildingRequirementRepository>;
}
