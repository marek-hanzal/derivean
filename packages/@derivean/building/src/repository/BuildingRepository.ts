import {
    type Database,
    withConnection
}                       from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                       from "@use-pico/server";
import {BuildingSchema} from "../schema/BuildingSchema";

export class BuildingRepository extends AbstractRepository<
    Database,
    BuildingSchema,
    "Building"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
            BuildingSchema,
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
