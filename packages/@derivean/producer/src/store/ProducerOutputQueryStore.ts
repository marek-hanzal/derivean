"use client";

import {createQueryStore}     from "@use-pico/client";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export const ProducerOutputQueryStore = createQueryStore({
    schema: ProducerOutputSchema.query,
});
