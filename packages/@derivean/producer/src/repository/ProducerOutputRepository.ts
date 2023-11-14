import {type Database}        from "@derivean/orm";
import {lazyOf}               from "@use-pico/container";
import {
    type Client,
    withClient
}                             from "@use-pico/orm";
import {AbstractRepository}   from "@use-pico/repository";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export class ProducerOutputRepository extends AbstractRepository<
    Database,
    ProducerOutputSchema,
    "ProducerOutput"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ProducerOutputSchema,
            "ProducerOutput",
        );
        this.defaultOrderBy = {
            amount: "asc",
        };
        this.matchOf = {
            producerId: "producerId",
            itemId: "itemId",
        };
    }
}

export namespace ProducerOutputRepository {
    export type Type = InstanceType<typeof ProducerOutputRepository>;
}
