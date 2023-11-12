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
import {ResourceSchema}     from "../schema/ResourceSchema";

export class ResourceRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ResourceSchema>,
    "Resource"
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
            ResourceSchema.repository,
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
