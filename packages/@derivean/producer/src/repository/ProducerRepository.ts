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
import {ProducerSchema}     from "../schema/ProducerSchema";

export class ProducerRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ProducerSchema>,
    "Producer"
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
            ProducerSchema.repository,
            "Producer",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name: "name",
        };
    }
}

export namespace ProducerRepository {
    export type Type = InstanceType<typeof ProducerRepository>;
}
