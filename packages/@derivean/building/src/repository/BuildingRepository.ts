import {type Database}            from "@derivean/orm";
import {
    type Client,
    withClient
}                                 from "@use-pico/orm";
import {AbstractRepository}       from "@use-pico/repository";
import {BuildingRepositorySchema} from "../schema/BuildingRepositorySchema";

export class BuildingRepository extends AbstractRepository<Database, BuildingRepositorySchema, "Building"> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            BuildingRepositorySchema,
            "Building",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name: "name",
        };
    }
}

export namespace BuildingRepository {
    export type Type = InstanceType<typeof BuildingRepository>;
}
