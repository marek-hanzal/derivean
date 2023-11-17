import {
    type Database,
    withConnection
}                        from "@derivean/orm";
import {
    AbstractRepository,
    type Connection,
    lazyOf
}                        from "@use-pico/server";
import {InventorySchema} from "../schema/InventorySchema";

export class InventoryRepository extends AbstractRepository<
    Database,
    InventorySchema,
    "Inventory"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: Connection<Database>,
    ) {
        super(
            connection,
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
