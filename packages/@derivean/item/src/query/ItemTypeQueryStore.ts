"use client";

import {createQueryStore} from "@use-pico/query";
import {ItemTypeSchema}   from "../schema/ItemTypeSchema";

export const ItemTypeQueryStore = createQueryStore({
    name:   "ItemTypeQueryStore",
    schema: ItemTypeSchema.query,
});
