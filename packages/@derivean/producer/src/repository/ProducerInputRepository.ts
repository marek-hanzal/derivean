import {type Database}                 from "@derivean/orm";
import {
    type Client,
    withClient
}                                      from "@use-pico/orm";
import {AbstractRepository}            from "@use-pico/repository";
import {ProducerInputRepositorySchema} from "../schema/input/ProducerInputRepositorySchema";

export class ProducerInputRepository extends AbstractRepository<Database, ProducerInputRepositorySchema, "ProducerInput"> {
    static inject = [
        withClient.inject,
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ProducerInputRepositorySchema,
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
