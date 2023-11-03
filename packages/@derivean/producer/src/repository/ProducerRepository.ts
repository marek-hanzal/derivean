import {type Database}            from "@derivean/orm";
import {
    type Client,
    withClient
}                                 from "@use-pico/orm";
import {AbstractRepository}       from "@use-pico/repository";
import {ProducerRepositorySchema} from "../schema/ProducerRepositorySchema";

export class ProducerRepository extends AbstractRepository<Database, ProducerRepositorySchema, "Producer"> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ProducerRepositorySchema,
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
