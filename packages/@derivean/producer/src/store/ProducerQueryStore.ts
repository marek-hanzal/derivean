"use client";

import {createQueryStore} from "@use-pico/client";
import {ProducerSchema}   from "../schema/ProducerSchema";

export const ProducerQueryStore = createQueryStore({
    schema: ProducerSchema.query,
});
