import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {ProducerSchema}     from "../schema/ProducerSchema";

export class ProducerRepository extends AbstractRepository<
    Database,
    ProducerSchema,
    "Producer"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
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
