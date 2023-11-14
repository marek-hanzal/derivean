import {type Database}             from "@derivean/orm";
import {lazyOf}                    from "@use-pico/container";
import {
    type Client,
    withClient
}                                  from "@use-pico/orm";
import {AbstractRepository}        from "@use-pico/repository";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export class BuildingRequirementRepository extends AbstractRepository<
    Database,
    BuildingRequirementSchema,
    "BuildingRequirement"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            BuildingRequirementSchema,
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
