import {type Database}      from "@derivean/orm";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {BuildingSchema}     from "../schema/BuildingSchema";

export class BuildingRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<BuildingSchema>,
    "Building"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
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
            name: "name",
        };
    }
}

export namespace BuildingRepository {
    export type Type = InstanceType<typeof BuildingRepository>;
}
