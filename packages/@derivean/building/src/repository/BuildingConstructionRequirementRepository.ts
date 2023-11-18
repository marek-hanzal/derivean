import {
    type Database,
    withConnection
}                                              from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                                              from "@use-pico/server";
import {BuildingConstructionRequirementSchema} from "../schema/BuildingConstructionRequirementSchema";

export class BuildingConstructionRequirementRepository extends AbstractRepository<
    Database,
    BuildingConstructionRequirementSchema,
    "BuildingConstructionRequirement"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
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
