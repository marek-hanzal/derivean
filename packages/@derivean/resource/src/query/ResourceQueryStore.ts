"use client";

import {createQueryStore}    from "@use-pico2/query";
import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";

export const ResourceQueryStore = createQueryStore({
    name:   "ResourceQueryStore",
    schema: ResourceQuerySchema,
});
