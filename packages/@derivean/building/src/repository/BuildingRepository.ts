import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {BuildingSchema}     from "../schema/BuildingSchema";

export class BuildingRepository extends AbstractRepository<
    Database,
    BuildingSchema,
    "Building"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
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
            name: "name",
        };
    }
}

export namespace BuildingRepository {
    export type Type = InstanceType<typeof BuildingRepository>;
}
