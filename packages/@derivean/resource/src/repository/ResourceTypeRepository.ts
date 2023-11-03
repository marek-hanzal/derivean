import {type Database}                from "@derivean/orm";
import {
    type Client,
    withClient
}                                     from "@use-pico/orm";
import {AbstractRepository}           from "@use-pico/repository";
import {ResourceTypeRepositorySchema} from "../schema/type/ResourceTypeRepositorySchema";

export class ResourceTypeRepository extends AbstractRepository<Database, ResourceTypeRepositorySchema, "ResourceType"> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ResourceTypeRepositorySchema,
            "ResourceType",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name: "name",
        };
    }
}

export namespace ResourceTypeRepository {
    export type Type = InstanceType<typeof ResourceTypeRepository>;
}
