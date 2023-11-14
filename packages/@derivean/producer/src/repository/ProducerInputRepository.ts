import {type Database}       from "@derivean/orm";
import {lazyOf}              from "@use-pico/container";
import {
    type Client,
    withClient
}                            from "@use-pico/orm";
import {AbstractRepository}  from "@use-pico/repository";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export class ProducerInputRepository extends AbstractRepository<
    Database,
    ProducerInputSchema,
    "ProducerInput"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            ProducerInputSchema,
            "ProducerInput",
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

export namespace ProducerInputRepository {
    export type Type = InstanceType<typeof ProducerInputRepository>;
}
