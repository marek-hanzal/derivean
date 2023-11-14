import {type Database}                         from "@derivean/orm";
import {lazyOf}                                from "@use-pico/container";
import {
    type Client,
    withClient
}                                              from "@use-pico/orm";
import {AbstractRepository}                    from "@use-pico/repository";
import {BuildingConstructionRequirementSchema} from "../schema/BuildingConstructionRequirementSchema";

export class BuildingConstructionRequirementRepository extends AbstractRepository<
    Database,
    BuildingConstructionRequirementSchema,
    "BuildingConstructionRequirement"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            BuildingConstructionRequirementSchema,
            "BuildingConstructionRequirement",
        );
        this.defaultOrderBy = {
            buildingId: "asc",
        };
        this.matchOf = {
            itemId:     "itemId",
            buildingId: "buildingId",
        };
    }
}

export namespace BuildingConstructionRequirementRepository {
    export type Type = InstanceType<typeof BuildingConstructionRequirementRepository>;
}
