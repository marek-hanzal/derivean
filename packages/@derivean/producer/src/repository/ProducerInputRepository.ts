import {
    type Database,
    withConnection
}                            from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                            from "@use-pico/server";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export class ProducerInputRepository extends AbstractRepository<
    Database,
    ProducerInputSchema,
    "ProducerInput"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
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
