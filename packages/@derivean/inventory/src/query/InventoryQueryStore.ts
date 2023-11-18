"use client";


import {createQueryStore} from "@use-pico/client";
import {InventorySchema}  from "../schema/InventorySchema";

export const InventoryQueryStore = createQueryStore({
    name:   "InventoryQueryStore",
    schema: InventorySchema.query,
});
