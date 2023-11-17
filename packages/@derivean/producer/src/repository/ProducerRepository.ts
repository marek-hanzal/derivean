import {type Database}  from "@derivean/orm";
import {
    AbstractRepository,
    type Connection,
    lazyOf,
    withConnection
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
        connection: Connection<Database>,
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
