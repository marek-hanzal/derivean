import {type Database}      from "@derivean/orm";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {
    type IRedisService,
    withRedisService
}                           from "@use-pico/redis";
import {AbstractRepository} from "@use-pico/repository";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export class ResourceTypeRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ResourceTypeSchema>,
    "ResourceType"
> {
    static inject = [
        withClient.inject,
        withRedisService.inject,
    ];

    constructor(
        client: Client<Database>,
        redisService: IRedisService,
    ) {
        super(
            client,
            redisService,
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
