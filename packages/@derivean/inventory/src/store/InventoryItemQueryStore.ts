"use client";

import {createQueryStore}    from "@use-pico/client";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export const InventoryItemQueryStore = createQueryStore({
    name:   "InventoryItemQueryStore",
    schema: InventoryItemSchema.query,
});
export const {
    Provider: InventoryItemQueryProvider,
} = InventoryItemQueryStore;
