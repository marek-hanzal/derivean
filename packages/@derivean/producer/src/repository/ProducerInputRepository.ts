import {type Database}       from "@derivean/orm";
import {withDullSchema}      from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {
    IRedisService,
    withRedisService
}                            from "@use-pico/redis";
import {AbstractRepository}  from "@use-pico/repository";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export class ProducerInputRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ProducerInputSchema>,
    "ProducerInput"
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
            ProducerInputSchema.repository,
            "ProducerInput",
        );
        this.defaultOrderBy = {
            amount: "asc",
        };
        this.matchOf = {
            producerId: "producerId",
            resourceId: "resourceId",
        };
    }
}

export namespace ProducerInputRepository {
    export type Type = InstanceType<typeof ProducerInputRepository>;
}
