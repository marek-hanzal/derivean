import {type Database}      from "@derivean/orm";
import {ProducerRepository} from "@derivean/producer";
import {lazyOf}             from "@use-pico/container";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {InventorySchema}    from "../schema/InventorySchema";

export class InventoryRepository extends AbstractRepository<
    Database,
    withDullSchema.Infer.RepositorySchema<InventorySchema>,
    "Inventory"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
        protected producerRepository: ProducerRepository.Type,
    ) {
        super(
            client,
            InventorySchema.repository,
            "Inventory",
        );
        this.defaultOrderBy = {
            name: "asc",
        };
        this.matchOf = {
            name: "name",
        };
    }
}

export namespace InventoryRepository {
    export type Type = InstanceType<typeof InventoryRepository>;
}
