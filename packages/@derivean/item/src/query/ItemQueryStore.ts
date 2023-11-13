"use client";

import {createQueryStore} from "@use-pico/query";
import {ItemSchema}       from "../schema/ItemSchema";

export const ItemQueryStore = createQueryStore({
    name:   "ItemQueryStore",
    schema: ItemSchema.query,
});
