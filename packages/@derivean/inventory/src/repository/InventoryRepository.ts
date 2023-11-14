import {type Database}      from "@derivean/orm";
import {lazyOf}             from "@use-pico/container";
import {
    type Client,
    withClient
}                           from "@use-pico/orm";
import {AbstractRepository} from "@use-pico/repository";
import {InventorySchema}    from "../schema/InventorySchema";

export class InventoryRepository extends AbstractRepository<
    Database,
    InventorySchema,
    "Inventory"
> {
    static inject = [
        lazyOf(withClient.inject),
    ];

    constructor(
        client: Client<Database>,
    ) {
        super(
            client,
            InventorySchema,
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
