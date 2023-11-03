"use client";

import {createQueryStore}    from "@use-pico/query";
import {ProducerQuerySchema} from "../schema/ProducerQuerySchema";

export const ProducerQueryStore = createQueryStore({
    schema: ProducerQuerySchema,
});
