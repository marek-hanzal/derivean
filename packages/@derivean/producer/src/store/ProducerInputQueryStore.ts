"use client";

import {createQueryStore}    from "@use-pico/client";
import {ProducerInputSchema} from "../schema/ProducerInputSchema";

export const ProducerInputQueryStore = createQueryStore({
    schema: ProducerInputSchema.query,
});
