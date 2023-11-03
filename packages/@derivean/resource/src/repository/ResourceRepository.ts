import {type Database}            from "@derivean/orm";
import {
    type Client,
    withClient
}                                 from "@use-pico/orm";
import {AbstractRepository}       from "@use-pico/repository";
import {ResourceRepositorySchema} from "../schema/ResourceRepositorySchema";

export class ResourceRepository extends AbstractRepository<Database, ResourceRepositorySchema, "Resource"> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ResourceRepositorySchema,
            "Resource",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name:   "name",
            typeId: "typeId",
        };
    }
}

export namespace ResourceRepository {
    export type Type = InstanceType<typeof ResourceRepository>;
}
