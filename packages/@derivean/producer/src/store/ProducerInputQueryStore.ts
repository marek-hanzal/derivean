"use client";

import {createQueryStore}    from "@use-pico/query";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export const ProducerInputQueryStore = createQueryStore({
    schema: ProducerInputSchema.query,
});
