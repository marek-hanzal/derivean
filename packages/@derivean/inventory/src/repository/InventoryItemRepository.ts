import {
    type Database,
    withConnection
}                            from "@derivean/orm";
import {type Infer}          from "@use-pico/extras";
import {
    AbstractRepository,
    lazyOf,
    SelectOf
}                            from "@use-pico/server";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export class InventoryItemRepository extends AbstractRepository<
    Database,
    InventoryItemSchema,
    "InventoryItem"
> {
    static inject = [
        lazyOf(withConnection.inject),
    ];

    constructor(
        connection: withConnection,
    ) {
        super(
            connection,
            InventoryItemSchema,
            "InventoryItem",
        );
        this.defaultOrderBy = {
            amount:      "desc",
            "Item.name": "asc",
        };
        this.matchOf = {
            inventoryId: "inventoryId",
            itemId:      "itemId",
        };
    }

    public with<T>(query: Infer.Query<InventoryItemSchema>, select: SelectOf<Database, "InventoryItem", T>): SelectOf<Database, "InventoryItem", T> {
        let $select = select;

        $select = $select.innerJoin("Item", "Item.id", "InventoryItem.itemId" as any);

        return $select;
    }
}

export namespace InventoryItemRepository {
    export type Type = InstanceType<typeof InventoryItemRepository>;
}
