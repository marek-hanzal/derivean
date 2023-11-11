"use client";

import {createQueryStore} from "@use-pico/query";
import {ProducerSchema}   from "../schema/ProducerSchema";

export const ProducerQueryStore = createQueryStore({
    schema: ProducerSchema.query,
});
