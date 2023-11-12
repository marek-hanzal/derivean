import {type Database}        from "@derivean/orm";
import {withDullSchema}       from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                             from "@use-pico/orm";
import {
    IRedisService,
    withRedisService
}                             from "@use-pico/redis";
import {AbstractRepository}   from "@use-pico/repository";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export class ProducerOutputRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<ProducerOutputSchema>,
    "ProducerOutput"
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
            ProducerOutputSchema.repository,
            "ProducerOutput",
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

export namespace ProducerOutputRepository {
    export type Type = InstanceType<typeof ProducerOutputRepository>;
}