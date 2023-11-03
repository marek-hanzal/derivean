"use client";

import {createQueryStore}        from "@use-pico/query";
import {ResourceTypeQuerySchema} from "../../schema/type/ResourceTypeQuerySchema";

export const ResourceTypeQueryStore = createQueryStore({
    schema: ResourceTypeQuerySchema,
});
