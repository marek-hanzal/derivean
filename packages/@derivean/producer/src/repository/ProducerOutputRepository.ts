import {
    type Database,
    withConnection
}                             from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                             from "@use-pico/server";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export class ProducerOutputRepository extends AbstractRepository<
    Database,
    ProducerOutputSchema,
    "ProducerOutput"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
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
