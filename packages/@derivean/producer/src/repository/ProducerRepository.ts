import {
    type Database,
    withConnection
}                       from "@derivean/orm";
import {
    AbstractRepository,
    lazyOf
}                       from "@use-pico/server";
import {ProducerSchema} from "../schema/ProducerSchema";

export class ProducerRepository extends AbstractRepository<
    Database,
    ProducerSchema,
    "Producer"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
            ProducerSchema,
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
