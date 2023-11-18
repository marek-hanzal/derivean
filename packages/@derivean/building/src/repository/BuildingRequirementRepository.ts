import {
    type Database,
    withConnection
}                                  from "@derivean/orm";
import {
    AbstractRepository,
    type Connection,
    lazyOf
}                                  from "@use-pico/server";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export class BuildingRequirementRepository extends AbstractRepository<
    Database,
    BuildingRequirementSchema,
    "BuildingRequirement"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: Connection<Database>,
    ) {
        super(
            connection,
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
