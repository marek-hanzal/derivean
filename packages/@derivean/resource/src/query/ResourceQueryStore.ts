"use client";

import {createQueryStore}    from "@use-pico/query";
import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";

export const ResourceQueryStore = createQueryStore({
    schema: ResourceQuerySchema,
});
