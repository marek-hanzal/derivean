"use client";

import {createQueryStore} from "@use-pico/query";
import {ResourceSchema}   from "../schema/ResourceSchema";

export const ResourceQueryStore = createQueryStore({
    name: "ResourceQueryStore",
    schema: ResourceSchema.query,
});
