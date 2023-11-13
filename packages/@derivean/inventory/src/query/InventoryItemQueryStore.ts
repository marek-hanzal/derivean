"use client";

import {createQueryStore}    from "@use-pico/query";
import {InventoryItemSchema} from "../schema/InventoryItemSchema";

export const InventoryItemQueryStore = createQueryStore({
    name:   "InventoryItemQueryStore",
    schema: InventoryItemSchema.query,
});
