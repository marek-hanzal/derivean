import {type Database}      from "@derivean/orm";
import {dullSchema}         from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export class ResourceTypeRepository extends AbstractRepository<
    Database,
    dullSchema.Infer.Repository<ResourceTypeSchema>,
    "ResourceType"
> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ResourceTypeSchema.repository,
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
